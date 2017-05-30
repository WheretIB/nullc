#include "InstructionTreeVm.h"

#include "ExpressionTree.h"

// TODO: VM code generation should use a special pointer type to generate special pointer instructions
#ifdef _M_X64
	#define VM_INST_LOAD_POINTER VM_INST_LOAD_LONG
#else
	#define VM_INST_LOAD_POINTER VM_INST_LOAD_INT
#endif

namespace
{
	bool IsMemberScope(ScopeData *scope)
	{
		return scope->ownerType != NULL;
	}

	bool IsGlobalScope(ScopeData *scope)
	{
		// Not a global scope if there is an enclosing function or a type
		while(scope)
		{
			if(scope->ownerFunction || scope->ownerType)
				return false;

			scope = scope->scope;
		}

		return true;
	}

	bool DoesConstantIntegerMatch(VmValue* value, long long number)
	{
		if(VmConstant *constant = getType<VmConstant>(value))
		{
			if(constant->type == VmType::Int)
				return constant->iValue == number;

			if(constant->type == VmType::Long)
				return constant->lValue == number;
		}

		return false;
	}

	bool DoesConstantMatchEither(VmValue* value, int iValue, double dValue, long long lValue)
	{
		if(VmConstant *constant = getType<VmConstant>(value))
		{
			if(constant->type == VmType::Int)
				return constant->iValue == iValue;

			if(constant->type == VmType::Double)
				return constant->dValue == dValue;

			if(constant->type == VmType::Long)
				return constant->lValue == lValue;
		}

		return false;
	}

	bool IsConstantZero(VmValue* value)
	{
		return DoesConstantMatchEither(value, 0, 0.0, 0ll);
	}

	bool IsConstantOne(VmValue* value)
	{
		return DoesConstantMatchEither(value, 1, 1.0, 1ll);
	}

	VmValue* CheckType(ExpressionContext &ctx, ExprBase* expr, VmValue *value)
	{
		VmType exprType = GetVmType(ctx, expr->type);

		assert(exprType == value->type);

		return value;
	}

	VmValue* CreateConstantInt(int value)
	{
		VmConstant *result = new VmConstant(VmType::Int);

		result->iValue = value;

		return result;
	}

	VmValue* CreateConstantDouble(double value)
	{
		VmConstant *result = new VmConstant(VmType::Double);

		result->dValue = value;

		return result;
	}

	VmValue* CreateConstantLong(long long value)
	{
		VmConstant *result = new VmConstant(VmType::Long);

		result->lValue = value;

		return result;
	}

	VmValue* CreateConstantPointer(int value, bool isFrameOffset)
	{
		VmConstant *result = new VmConstant(VmType::Pointer);

		result->iValue = value;
		result->isFrameOffset = isFrameOffset;

		return result;
	}

	VmValue* CreateConstantStruct(char *value, int size)
	{
		assert(size % 4 == 0);

		VmConstant *result = new VmConstant(VmType::Struct(size));

		result->sValue = value;

		return result;
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

	VmInstruction* CreateInstruction(VmModule *module, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third, VmValue *fourth)
	{
		assert(module->currentBlock);

		VmInstruction *inst = new VmInstruction(type, cmd, module->nextInstructionId++);

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

	VmInstruction* CreateInstruction(VmModule *module, VmType type, VmInstructionType cmd)
	{
		return CreateInstruction(module, type, cmd, NULL, NULL, NULL, NULL);
	}

	VmInstruction* CreateInstruction(VmModule *module, VmType type, VmInstructionType cmd, VmValue *first)
	{
		return CreateInstruction(module, type, cmd, first, NULL, NULL, NULL);
	}

	VmInstruction* CreateInstruction(VmModule *module, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second)
	{
		return CreateInstruction(module, type, cmd, first, second, NULL, NULL);
	}

	VmInstruction* CreateInstruction(VmModule *module, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third)
	{
		return CreateInstruction(module, type, cmd, first, second, third, NULL);
	}

	VmValue* CreateLoad(ExpressionContext &ctx, VmModule *module, TypeBase *type, VmValue *address)
	{
		if(type == ctx.typeBool || type == ctx.typeChar)
			return CreateInstruction(module, VmType::Int, VM_INST_LOAD_BYTE, address);

		if(type == ctx.typeShort)
			return CreateInstruction(module, VmType::Int, VM_INST_LOAD_SHORT, address);

		if(type == ctx.typeInt)
			return CreateInstruction(module, VmType::Int, VM_INST_LOAD_INT, address);

		if(type == ctx.typeFloat)
			return CreateInstruction(module, VmType::Double, VM_INST_LOAD_FLOAT, address);

		if(type == ctx.typeDouble)
			return CreateInstruction(module, VmType::Double, VM_INST_LOAD_DOUBLE, address);

		if(type == ctx.typeLong)
			return CreateInstruction(module, VmType::Long, VM_INST_LOAD_LONG, address);

		if(isType<TypeRef>(type))
			return CreateInstruction(module, VmType::Pointer, VM_INST_LOAD_POINTER, address);

		if(isType<TypeFunction>(type))
			return CreateInstruction(module, VmType::FunctionRef, VM_INST_LOAD_STRUCT, address);

		if(isType<TypeUnsizedArray>(type))
			return CreateInstruction(module, VmType::ArrayRef, VM_INST_LOAD_STRUCT, address);

		if(type == ctx.typeAutoRef)
			return CreateInstruction(module, VmType::AutoRef, VM_INST_LOAD_STRUCT, address);

		if(type == ctx.typeAutoArray)
			return CreateInstruction(module, VmType::AutoArray, VM_INST_LOAD_STRUCT, address);

		if(isType<TypeTypeID>(type))
			return CreateInstruction(module, VmType::Int, VM_INST_LOAD_INT, address);

		if(type->size == 0)
			return CreateConstantInt(0);

		assert(type->size % 4 == 0);
		assert(type->size != 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return CreateInstruction(module, VmType::Struct(type->size), VM_INST_LOAD_STRUCT, address);
	}

	VmValue* CreateStore(ExpressionContext &ctx, VmModule *module, TypeBase *type, VmValue *address, VmValue *value)
	{
		if(type == ctx.typeBool || type == ctx.typeChar)
		{
			assert(value->type == VmType::Int);

			return CreateInstruction(module, VmType::Void, VM_INST_STORE_BYTE, address, value);
		}

		if(type == ctx.typeShort)
		{
			assert(value->type == VmType::Int);

			return CreateInstruction(module, VmType::Void, VM_INST_STORE_SHORT, address, value);
		}

		if(type == ctx.typeInt)
		{
			assert(value->type == VmType::Int);

			return CreateInstruction(module, VmType::Void, VM_INST_STORE_INT, address, value);
		}

		if(type == ctx.typeFloat)
		{
			assert(value->type == VmType::Double);

			return CreateInstruction(module, VmType::Void, VM_INST_STORE_FLOAT, address, value);
		}

		if(type == ctx.typeDouble)
		{
			assert(value->type == VmType::Double);

			return CreateInstruction(module, VmType::Void, VM_INST_STORE_DOUBLE, address, value);
		}

		if(type == ctx.typeLong)
		{
			assert(value->type == VmType::Long);

			return CreateInstruction(module, VmType::Void, VM_INST_STORE_LONG, address, value);
		}

		if(type->size == 0)
			return new VmVoid();

		assert(type->size % 4 == 0);
		assert(type->size != 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);
		assert(value->type == GetVmType(ctx, type));

		return CreateInstruction(module, VmType::Void, VM_INST_STORE_STRUCT, address, value);
	}

	VmValue* CreateCast(VmModule *module, VmValue *value, VmType target)
	{
		if(target == value->type)
			return value;

		if(target == VmType::Int)
		{
			if(value->type == VmType::Double)
				return CreateInstruction(module, target, VM_INST_DOUBLE_TO_INT, value);

			if(value->type == VmType::Long)
				return CreateInstruction(module, target, VM_INST_LONG_TO_INT, value);
		}
		else if(target == VmType::Double)
		{
			if(value->type == VmType::Int)
				return CreateInstruction(module, target, VM_INST_INT_TO_DOUBLE, value);

			if(value->type == VmType::Long)
				return CreateInstruction(module, target, VM_INST_LONG_TO_DOUBLE, value);
		}
		else if(target == VmType::Long)
		{
			if(value->type == VmType::Int)
				return CreateInstruction(module, target, VM_INST_INT_TO_LONG, value);

			if(value->type == VmType::Double)
				return CreateInstruction(module, target, VM_INST_DOUBLE_TO_LONG, value);
		}

		assert(!"unknown cast");

		return new VmVoid();
	}

	VmValue* CreateIndex(VmModule *module, VmValue *arrayLength, VmValue *elementSize, VmValue *value, VmValue *index)
	{
		assert(arrayLength->type == VmType::Int);
		assert(elementSize->type == VmType::Int);
		assert(value->type == VmType::Pointer);
		assert(index->type == VmType::Int);

		return CreateInstruction(module, VmType::Pointer, VM_INST_INDEX, arrayLength, elementSize, value, index);
	}

	VmValue* CreateIndexUnsized(VmModule *module, VmValue *elementSize, VmValue *value, VmValue *index)
	{
		assert(value->type == VmType::ArrayRef);
		assert(elementSize->type == VmType::Int);
		assert(index->type == VmType::Int);

		return CreateInstruction(module, VmType::Pointer, VM_INST_INDEX_UNSIZED, elementSize, value, index);
	}

	VmValue* CreateAdd(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		if(lhs->type == VmType::Pointer)
		{
			assert(rhs->type == VmType::Int);

			return CreateInstruction(module, VmType::Pointer, VM_INST_ADD, lhs, rhs);
		}

		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_ADD, lhs, rhs);
	}

	VmValue* CreateSub(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_SUB, lhs, rhs);
	}

	VmValue* CreateMul(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_MUL, lhs, rhs);
	}

	VmValue* CreateDiv(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_DIV, lhs, rhs);
	}

	VmValue* CreatePow(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_POW, lhs, rhs);
	}

	VmValue* CreateMod(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_MOD, lhs, rhs);
	}

	VmValue* CreateCompareLess(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_LESS, lhs, rhs);
	}

	VmValue* CreateCompareGreater(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_GREATER, lhs, rhs);
	}

	VmValue* CreateCompareLessEqual(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_LESS_EQUAL, lhs, rhs);
	}

	VmValue* CreateCompareGreaterEqual(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_GREATER_EQUAL, lhs, rhs);
	}

	VmValue* CreateCompareEqual(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_EQUAL, lhs, rhs);
	}

	VmValue* CreateCompareNotEqual(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		// Some comparisons with null pointer are allowed
		if((lhs->type == VmType::FunctionRef || lhs->type == VmType::ArrayRef || lhs->type == VmType::AutoRef) && rhs->type == VmType::Pointer && isType<VmConstant>(rhs) && ((VmConstant*)rhs)->iValue == 0)
			return CreateInstruction(module, VmType::Int, VM_INST_NOT_EQUAL, lhs, rhs);

		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_NOT_EQUAL, lhs, rhs);
	}

	VmValue* CreateShl(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_SHL, lhs, rhs);
	}

	VmValue* CreateShr(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_SHR, lhs, rhs);
	}

	VmValue* CreateAnd(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_BIT_AND, lhs, rhs);
	}

	VmValue* CreateOr(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_BIT_OR, lhs, rhs);
	}

	VmValue* CreateXor(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, lhs->type, VM_INST_BIT_XOR, lhs, rhs);
	}

	VmValue* CreateLogicalAnd(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_LOG_AND, lhs, rhs);
	}

	VmValue* CreateLogicalOr(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_LOG_OR, lhs, rhs);
	}

	VmValue* CreateLogicalXor(VmModule *module, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, VmType::Int, VM_INST_LOG_XOR, lhs, rhs);
	}

	VmValue* CreateNeg(VmModule *module, VmValue *value)
	{
		assert(value->type == VmType::Int || value->type == VmType::Double || value->type == VmType::Long);

		return CreateInstruction(module, value->type, VM_INST_NEG, value);
	}

	VmValue* CreateNot(VmModule *module, VmValue *value)
	{
		assert(value->type == VmType::Int || value->type == VmType::Long);

		return CreateInstruction(module, value->type, VM_INST_BIT_NOT, value);
	}

	VmValue* CreateLogicalNot(VmModule *module, VmValue *value)
	{
		assert(value->type == VmType::Int || value->type == VmType::Long || value->type == VmType::Pointer || value->type == VmType::AutoRef);

		return CreateInstruction(module, VmType::Int, VM_INST_LOG_NOT, value);
	}

	VmValue* CreateJump(VmModule *module, VmValue *label)
	{
		assert(label->type == VmType::Label);

		return CreateInstruction(module, VmType::Void, VM_INST_JUMP, label);
	}

	VmValue* CreateJumpZero(VmModule *module, VmValue *value, VmValue *trueLabel, VmValue *falseLabel)
	{
		assert(value->type == VmType::Int);
		assert(trueLabel->type == VmType::Label);
		assert(falseLabel->type == VmType::Label);

		return CreateInstruction(module, VmType::Void, VM_INST_JUMP_Z, value, trueLabel, falseLabel);
	}

	VmValue* CreateJumpNotZero(VmModule *module, VmValue *value, VmValue *trueLabel, VmValue *falseLabel)
	{
		assert(value->type == VmType::Int);
		assert(trueLabel->type == VmType::Label);
		assert(falseLabel->type == VmType::Label);

		return CreateInstruction(module, VmType::Void, VM_INST_JUMP_NZ, value, trueLabel, falseLabel);
	}

	VmValue* CreateReturn(VmModule *module)
	{
		return CreateInstruction(module, VmType::Void, VM_INST_RETURN);
	}

	VmValue* CreateReturn(VmModule *module, VmValue *value)
	{
		return CreateInstruction(module, VmType::Void, VM_INST_RETURN, value);
	}

	VmValue* CreateYield(VmModule *module, VmValue *value)
	{
		return CreateInstruction(module, VmType::Void, VM_INST_YIELD, value);
	}

	VmValue* CreateVariableAddress(VariableData *variable)
	{
		assert(!IsMemberScope(variable->scope));

		bool isFrameOffset = !IsGlobalScope(variable->scope);

		VmValue *value = CreateConstantPointer(variable->offset, isFrameOffset);

		value->comment = variable->name;

		return value;
	}

	VmValue* CreateTypeIndex(VmModule *module, TypeBase *type)
	{
		return CreateInstruction(module, VmType::Int, VM_INST_TYPE_ID, CreateConstantInt(type->typeIndex));
	}

	VmValue* CreateConvertPtr(VmModule *module, VmValue *ptr, TypeBase *type)
	{
		return CreateInstruction(module, VmType::Pointer, VM_INST_CONVERT_POINTER, ptr, CreateTypeIndex(module, type));
	}

	VmValue* CreateConstruct(VmModule *module, VmType type, VmValue *el0, VmValue *el1, VmValue *el2, VmValue *el3)
	{
		unsigned size = el0->type.size;

		if(el1)
			size += el1->type.size;

		if(el2)
			size += el2->type.size;

		if(el3)
			size += el3->type.size;

		assert(type.size == size);

		return CreateInstruction(module, type, VM_INST_CONSTRUCT, el0, el1, el2, el3);
	}

	VmValue* AllocateScopeVariable(ExpressionContext &ctx, VmModule *module, TypeBase *type)
	{
		FunctionData *function = module->currentFunction->function;

		ScopeData *scope = NULL;
		unsigned offset = 0;

		if(function)
		{
			scope = function->scope;

			function->stackSize += GetAlignmentOffset(function->stackSize, type->alignment);

			offset = unsigned(function->stackSize);

			function->stackSize += type->size;
		}
		else
		{
			scope = ctx.globalScope;

			scope->globalSize += GetAlignmentOffset(scope->globalSize, type->alignment);

			offset = unsigned(scope->globalSize);

			scope->globalSize += type->size; // TODO: alignment
		}

		char *name = new char[16];
		sprintf(name, "$temp%d", ctx.unnamedVariableCount++);

		VariableData *variable = new VariableData(NULL, scope, type->alignment, type, InplaceStr(name), offset, 0);

		scope->variables.push_back(variable);

		ctx.variables.push_back(variable);

		return CreateVariableAddress(variable);
	}

	void ChangeInstructionTo(VmInstruction *inst, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third, VmValue *fourth, unsigned *optCount)
	{
		inst->cmd = cmd;

		SmallArray<VmValue*, 128> arguments;
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
			for(unsigned i = 0; i < inst->arguments.size(); i++)
			{
				if(inst->arguments[i] == original)
				{
					replacement->AddUse(inst);
					original->RemoveUse(inst);

					inst->arguments[i] = replacement;
				}
			}
		}
	}

	void ReplaceValueUsersWith(VmValue *original, VmValue *replacement, unsigned *optCount)
	{
		SmallArray<VmValue*, 256> users;
		users.reserve(original->users.size());
		users.push_back(original->users.data, original->users.size());

		for(unsigned i = 0; i < users.size(); i++)
			ReplaceValue(users[i], original, replacement);

		if(optCount)
			(*optCount)++;
	}

	unsigned GetAccessSize(VmInstruction *inst)
	{
		switch(inst->cmd)
		{
		case VM_INST_LOAD_BYTE:
			return 1;
		case VM_INST_LOAD_SHORT:
			return 2;
		case VM_INST_LOAD_INT:
			return 4;
		case VM_INST_LOAD_FLOAT:
			return 4;
		case VM_INST_LOAD_DOUBLE:
			return 8;
		case VM_INST_LOAD_LONG:
			return 8;
		case VM_INST_LOAD_STRUCT:
			return inst->type.size;
		case VM_INST_STORE_BYTE:
			return 1;
		case VM_INST_STORE_SHORT:
			return 2;
		case VM_INST_STORE_INT:
			return 4;
		case VM_INST_STORE_FLOAT:
			return 4;
		case VM_INST_STORE_DOUBLE:
			return 8;
		case VM_INST_STORE_LONG:
			return 8;
		case VM_INST_STORE_STRUCT:
			return inst->arguments[1]->type.size;
		}

		return 0;
	}

	void ClearLoadStoreInfo(VmModule *module)
	{
		module->loadStoreInfo.clear();
	}

	void ClearLoadStoreInfo(VmModule *module, bool isFrameOffset, unsigned storeOffset, unsigned storeSize)
	{
		assert(storeSize != 0);

		for(unsigned i = 0; i < module->loadStoreInfo.size();)
		{
			VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

			unsigned otherOffset = unsigned(el.address->iValue);
			unsigned otherSize = GetAccessSize(el.loadInst ? el.loadInst : el.storeInst);

			assert(otherSize != 0);

			// (a+aw >= b) && (a <= b+bw)
			if(isFrameOffset == el.address->isFrameOffset && storeOffset + storeSize - 1 >= otherOffset && storeOffset <= otherOffset + otherSize - 1)
			{
				module->loadStoreInfo[i] = module->loadStoreInfo.back();
				module->loadStoreInfo.pop_back();
			}
			else
			{
				i++;
			}
		}
	}

	void AddLoadInfo(VmModule *module, VmInstruction* inst)
	{
		if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
		{
			VmModule::LoadStoreInfo info;

			info.loadInst = inst;
			info.storeInst = NULL;

			info.address = address;

			module->loadStoreInfo.push_back(info);
		}
	}

	void AddStoreInfo(VmModule *module, VmInstruction* inst)
	{
		if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
		{
			VmModule::LoadStoreInfo info;

			info.loadInst = NULL;
			info.storeInst = inst;

			info.address = address;

			// Remove previous loads and stores to this address range
			ClearLoadStoreInfo(module, address->isFrameOffset, unsigned(address->iValue), GetAccessSize(inst));

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

						ClearLoadStoreInfo(module, base->isFrameOffset, storeOffset, storeSize);
						return;
					}
				}
			}

			ClearLoadStoreInfo(module);
		}
	}

	VmValue* GetLoadStoreInfo(VmModule *module, VmInstruction* inst)
	{
		if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
		{
			for(unsigned i = 0; i < module->loadStoreInfo.size(); i++)
			{
				VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

				if(el.loadInst && *el.address == *address && GetAccessSize(inst) == GetAccessSize(el.loadInst))
					return el.loadInst;

				if(el.storeInst && *el.address == *address && GetAccessSize(inst) == GetAccessSize(el.storeInst))
					return el.storeInst->arguments[1];
			}
		}

		return NULL;
	}
}

const VmType VmType::Void = VmType(VM_TYPE_VOID, 0);
const VmType VmType::Int = VmType(VM_TYPE_INT, 4);
const VmType VmType::Double = VmType(VM_TYPE_DOUBLE, 8);
const VmType VmType::Long = VmType(VM_TYPE_LONG, 8);
const VmType VmType::Label = VmType(VM_TYPE_LABEL, 4);
const VmType VmType::Pointer = VmType(VM_TYPE_POINTER, NULLC_PTR_SIZE);
const VmType VmType::FunctionRef = VmType(VM_TYPE_FUNCTION_REF, NULLC_PTR_SIZE + 4); // context + id
const VmType VmType::ArrayRef = VmType(VM_TYPE_ARRAY_REF, NULLC_PTR_SIZE + 4); // ptr + length
const VmType VmType::AutoRef = VmType(VM_TYPE_AUTO_REF, 4 + NULLC_PTR_SIZE); // type + ptr
const VmType VmType::AutoArray = VmType(VM_TYPE_AUTO_ARRAY, 4 + NULLC_PTR_SIZE + 4); // type + ptr + length

void VmValue::AddUse(VmValue* user)
{
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

	if(users.empty() && !hasSideEffects)
	{
		if(VmInstruction *instruction = getType<VmInstruction>(this))
		{
			instruction->parent->RemoveInstruction(instruction);
		}
		else if(VmBlock *block = getType<VmBlock>(this))
		{
			// Remove all block instructions
			while(block->lastInstruction)
				block->RemoveInstruction(block->lastInstruction);
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

	instruction->parent = this;

	if(!firstInstruction)
	{
		firstInstruction = lastInstruction = instruction;
	}else{
		lastInstruction->nextSibling = instruction;
		instruction->prevSibling = lastInstruction;
		lastInstruction = instruction;
	}
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

	block->parent = this;

	if(!firstBlock)
	{
		firstBlock = lastBlock = block;
	}else{
		lastBlock->nextSibling = block;
		block->prevSibling = lastBlock;
		lastBlock = block;
	}
}

void VmFunction::RemoveBlock(VmBlock* block)
{
	assert(block);
	assert(block->parent == this);
	assert(block->users.empty());

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

	while(block->lastInstruction)
		block->RemoveInstruction(block->lastInstruction);
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
		return VmType::Pointer;

	if(isType<TypeFunction>(type))
		return VmType::FunctionRef;

	if(isType<TypeUnsizedArray>(type))
		return VmType::ArrayRef;

	if(isType<TypeAutoRef>(type))
		return VmType::AutoRef;

	if(isType<TypeAutoArray>(type))
		return VmType::AutoArray;

	if(isType<TypeTypeID>(type))
		return VmType::Int;

	if(isType<TypeArray>(type) || isType<TypeClass>(type))
	{
		if(isType<TypeClass>(type) && type->size == 0)
			return VmType::Int;

		assert(type->size % 4 == 0);
		assert(type->size != 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return VmType::Struct(type->size);
	}

	assert(!"unknown type");

	return VmType::Void;
}

VmValue* CompileVm(ExpressionContext &ctx, VmModule *module, ExprBase *expression)
{
	if(ExprVoid *node = getType<ExprVoid>(expression))
	{
		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantInt(node->value ? 1 : 0));
	}
	else if(ExprCharacterLiteral *node = getType<ExprCharacterLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantInt(node->value));
	}
	else if(ExprStringLiteral *node = getType<ExprStringLiteral>(expression))
	{
		unsigned size = node->length + 1;

		// Align to 4
		size = (size + 3) & ~3;

		char *value = new char[size];
		memset(value, 0, size);

		for(unsigned i = 0; i < node->length; i++)
			value[i] = node->value[i];

		return CheckType(ctx, expression, CreateConstantStruct(value, size));
	}
	else if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(expression))
	{
		if(node->type == ctx.typeInt)
			return CheckType(ctx, expression, CreateConstantInt(int(node->value)));

		if(node->type == ctx.typeLong)
			return CheckType(ctx, expression, CreateConstantLong(node->value));

		assert(!"unknown type");
	}
	else if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantDouble(node->value));
	}
	else if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateTypeIndex(module, node->value));
	}
	else if(ExprNullptrLiteral *node = getType<ExprNullptrLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantPointer(0, false));
	}
	else if(ExprArray *node = getType<ExprArray>(expression))
	{
		VmValue *address = AllocateScopeVariable(ctx, module, node->type);

		TypeArray *arrayType = getType<TypeArray>(node->type);

		assert(arrayType);

		unsigned offset = 0;

		for(ExprBase *value = node->values.head; value; value = value->next)
		{
			VmValue *element = CompileVm(ctx, module, value);

			CreateStore(ctx, module, arrayType->subType, CreateAdd(module, address, CreateConstantInt(offset)), element);

			offset += unsigned(arrayType->subType->size);
		}

		return CheckType(ctx, expression, CreateLoad(ctx, module, node->type, address));
	}
	else if(ExprPreModify *node = getType<ExprPreModify>(expression))
	{
		VmValue *address = CompileVm(ctx, module, node->value);

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);

		VmValue *value = CreateLoad(ctx, module, refType->subType, address);

		if(value->type == VmType::Int)
			value = CreateAdd(module, value, CreateConstantInt(node->isIncrement ? 1 : -1));
		else if(value->type == VmType::Double)
			value = CreateAdd(module, value, CreateConstantDouble(node->isIncrement ? 1.0 : -1.0));
		else if(value->type == VmType::Long)
			value = CreateAdd(module, value, CreateConstantLong(node->isIncrement ? 1ll : -1ll));
		else
			assert("!unknown type");

		CreateStore(ctx, module, refType->subType, address, value);

		return CheckType(ctx, expression, value);

	}
	else if(ExprPostModify *node = getType<ExprPostModify>(expression))
	{
		VmValue *address = CompileVm(ctx, module, node->value);

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);

		VmValue *value = CreateLoad(ctx, module, refType->subType, address);
		VmValue *result = value;

		if(value->type == VmType::Int)
			value = CreateAdd(module, value, CreateConstantInt(node->isIncrement ? 1 : -1));
		else if(value->type == VmType::Double)
			value = CreateAdd(module, value, CreateConstantDouble(node->isIncrement ? 1.0 : -1.0));
		else if(value->type == VmType::Long)
			value = CreateAdd(module, value, CreateConstantLong(node->isIncrement ? 1ll : -1ll));
		else
			assert("!unknown type");

		CreateStore(ctx, module, refType->subType, address, value);

		return CheckType(ctx, expression, result);
	}
	else if(ExprTypeCast *node = getType<ExprTypeCast>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		switch(node->category)
		{
		case EXPR_CAST_NUMERICAL:
			return CheckType(ctx, expression, CreateCast(module, value, GetVmType(ctx, node->type)));
		case EXPR_CAST_PTR_TO_BOOL:
			return CheckType(ctx, expression, CreateCompareNotEqual(module, value, CreateConstantPointer(0, false)));
		case EXPR_CAST_UNSIZED_TO_BOOL:
			return CheckType(ctx, expression, CreateCompareNotEqual(module, value, CreateConstantPointer(0, false)));
		case EXPR_CAST_FUNCTION_TO_BOOL:
			return CheckType(ctx, expression, CreateCompareNotEqual(module, value, CreateConstantPointer(0, false)));
		case EXPR_CAST_NULL_TO_PTR:
			return CheckType(ctx, expression, CreateConstantPointer(0, false));
		case EXPR_CAST_NULL_TO_AUTO_PTR:
			return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), CreateConstantInt(0), CreateConstantPointer(0, false), NULL, NULL));
		case EXPR_CAST_NULL_TO_UNSIZED:
			return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), CreateConstantPointer(0, false), CreateConstantInt(0), NULL, NULL));
		case EXPR_CAST_NULL_TO_AUTO_ARRAY:
			return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), CreateConstantInt(0), CreateConstantPointer(0, false), CreateConstantInt(0), NULL));
		case EXPR_CAST_NULL_TO_FUNCTION:
			return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), CreateConstantPointer(0, false), CreateConstantInt(0), NULL, NULL));
		case EXPR_CAST_ARRAY_TO_UNSIZED:
			{
				TypeArray *arrType = getType<TypeArray>(node->value->type);

				assert(arrType);
				assert(unsigned(arrType->length) == arrType->length);

				VmValue *address = AllocateScopeVariable(ctx, module, arrType);

				CreateStore(ctx, module, arrType, address, value);

				return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), address, CreateConstantInt(unsigned(arrType->length)), NULL, NULL));
			}
			break;
		case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
			{
				TypeRef *refType = getType<TypeRef>(node->value->type);

				assert(refType);

				TypeArray *arrType = getType<TypeArray>(refType->subType);

				assert(arrType);
				assert(unsigned(arrType->length) == arrType->length);

				return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), value, CreateConstantInt(unsigned(arrType->length)), NULL, NULL));
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

				VmValue *address = AllocateScopeVariable(ctx, module, targetRefType->subType);

				CreateStore(ctx, module, targetRefType->subType, address, CreateConstruct(module, GetVmType(ctx, targetRefType->subType), address, CreateConstantInt(unsigned(arrType->length)), NULL, NULL));

				return CheckType(ctx, expression, address);
			}
			break;
		case EXPR_CAST_PTR_TO_AUTO_PTR:
			{
				TypeRef *refType = getType<TypeRef>(node->value->type);

				assert(refType);

				return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), CreateTypeIndex(module, refType->subType), value, NULL, NULL));
			}
			break;
		case EXPR_CAST_ANY_TO_PTR:
			{
				VmValue *address = AllocateScopeVariable(ctx, module, node->value->type);

				CreateStore(ctx, module, node->value->type, address, value);

				return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), address, NULL, NULL, NULL));
			}
			break;
		case EXPR_CAST_AUTO_PTR_TO_PTR:
			{
				TypeRef *refType = getType<TypeRef>(node->type);

				assert(refType);

				return CheckType(ctx, expression, CreateConvertPtr(module, value, refType->subType));
			}
		case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
			{
				TypeUnsizedArray *unsizedType = getType<TypeUnsizedArray>(node->value->type);

				assert(unsizedType);

				return CheckType(ctx, expression, CreateConstruct(module, GetVmType(ctx, node->type), CreateTypeIndex(module, unsizedType->subType), value, NULL, NULL));
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
			result = CreateNeg(module, value);
			break;
		case SYN_UNARY_OP_BIT_NOT:
			result = CreateNot(module, value);
			break;
		case SYN_UNARY_OP_LOGICAL_NOT:
			result = CreateLogicalNot(module, value);
			break;
		}

		assert(result);

		return CheckType(ctx, expression, result);
	}
	else if(ExprBinaryOp *node = getType<ExprBinaryOp>(expression))
	{
		VmValue *lhs = CompileVm(ctx, module, node->lhs);
		VmValue *rhs = CompileVm(ctx, module, node->rhs);

		VmValue *result = NULL;

		switch(node->op)
		{
		case SYN_BINARY_OP_ADD:
			result = CreateAdd(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_SUB:
			result = CreateSub(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_MUL:
			result = CreateMul(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_DIV:
			result = CreateDiv(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_MOD:
			result = CreateMod(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_POW:
			result = CreatePow(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_SHL:
			result = CreateShl(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_SHR:
			result = CreateShr(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_LESS:
			result = CreateCompareLess(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_LESS_EQUAL:
			result = CreateCompareLessEqual(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_GREATER:
			result = CreateCompareGreater(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_GREATER_EQUAL:
			result = CreateCompareGreaterEqual(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_EQUAL:
			result = CreateCompareEqual(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_NOT_EQUAL:
			result = CreateCompareNotEqual(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_BIT_AND:
			result = CreateAnd(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_BIT_OR:
			result = CreateOr(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_BIT_XOR:
			result = CreateXor(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_LOGICAL_AND:
			result = CreateLogicalAnd(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_LOGICAL_OR:
			result = CreateLogicalOr(module, lhs, rhs);
			break;
		case SYN_BINARY_OP_LOGICAL_XOR:
			result = CreateLogicalXor(module, lhs, rhs);
			break;
		}

		assert(result);

		return CheckType(ctx, expression, result);
	}
	else if(ExprGetAddress *node = getType<ExprGetAddress>(expression))
	{
		return CheckType(ctx, expression, CreateVariableAddress(node->variable));
	}
	else if(ExprDereference *node = getType<ExprDereference>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);
		assert(refType->subType == node->type);

		return CheckType(ctx, expression, CreateLoad(ctx, module, node->type, value));
	}
	else if(ExprConditional *node = getType<ExprConditional>(expression))
	{
		VmValue *address = AllocateScopeVariable(ctx, module, node->type);

		VmValue* condition = CompileVm(ctx, module, node->condition);

		VmBlock *trueBlock = new VmBlock(InplaceStr("if_true"), module->nextBlockId++);
		VmBlock *falseBlock = new VmBlock(InplaceStr("if_false"), module->nextBlockId++);
		VmBlock *exitBlock = new VmBlock(InplaceStr("if_exit"), module->nextBlockId++);

		if(node->falseBlock)
			CreateJumpNotZero(module, condition, trueBlock, falseBlock);
		else
			CreateJumpNotZero(module, condition, trueBlock, exitBlock);

		module->currentFunction->AddBlock(trueBlock);
		module->currentBlock = trueBlock;

		CreateStore(ctx, module, node->type, address, CompileVm(ctx, module, node->trueBlock));

		CreateJump(module, exitBlock);

		if(node->falseBlock)
		{
			module->currentFunction->AddBlock(falseBlock);
			module->currentBlock = falseBlock;

			CreateStore(ctx, module, node->type, address, CompileVm(ctx, module, node->falseBlock));

			CreateJump(module, exitBlock);
		}

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		return CheckType(ctx, expression, CreateLoad(ctx, module, node->type, address));
	}
	else if(ExprAssignment *node = getType<ExprAssignment>(expression))
	{
		TypeRef *refType = getType<TypeRef>(node->lhs->type);

		assert(refType);
		assert(refType->subType == node->rhs->type);

		VmValue *address = CompileVm(ctx, module, node->lhs);

		VmValue *initializer = CompileVm(ctx, module, node->rhs);

		CreateStore(ctx, module, node->rhs->type, address, initializer);

		return CheckType(ctx, expression, initializer);
	}
	else if(ExprMemberAccess *node = getType<ExprMemberAccess>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		assert(isType<TypeRef>(node->value->type));

		VmValue *offset = CreateConstantInt(node->member->offset);

		return CheckType(ctx, expression, CreateAdd(module, value, offset));
	}
	else if(ExprArrayIndex *node = getType<ExprArrayIndex>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);
		VmValue *index = CompileVm(ctx, module, node->index);

		if(TypeUnsizedArray *arrayType = getType<TypeUnsizedArray>(node->value->type))
		{
			VmValue *elementSize = CreateConstantInt(unsigned(arrayType->subType->size));

			return CheckType(ctx, expression, CreateIndexUnsized(module, elementSize, value, index));
		}

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);

		TypeArray *arrayType = getType<TypeArray>(refType->subType);

		assert(arrayType);
		assert(unsigned(arrayType->subType->size) == arrayType->subType->size);

		VmValue *arrayLength = CreateConstantInt(unsigned(arrayType->length));
		VmValue *elementSize = CreateConstantInt(unsigned(arrayType->subType->size));

		return CheckType(ctx, expression, CreateIndex(module, arrayLength, elementSize, value, index));
	}
	else if(ExprReturn *node = getType<ExprReturn>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		if(node->value->type == ctx.typeVoid)
			return CheckType(ctx, expression, CreateReturn(module));

		return CheckType(ctx, expression, CreateReturn(module, value));
	}
	else if(ExprYield *node = getType<ExprYield>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		return CheckType(ctx, expression, CreateYield(module, value));
	}
	else if(ExprVariableDefinition *node = getType<ExprVariableDefinition>(expression))
	{
		if(node->initializer)
			CompileVm(ctx, module, node->initializer);

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprArraySetup *node = getType<ExprArraySetup>(expression))
	{
		TypeArray *arrayType = getType<TypeArray>(node->variable->type);

		assert(arrayType);

		VmValue *initializer = CompileVm(ctx, module, node->initializer);

		VmValue *address = CreateVariableAddress(node->variable);

		// TODO: use cmdSetRange for supported types

		VmValue *offsetPtr = AllocateScopeVariable(ctx, module, ctx.typeInt);

		VmBlock *conditionBlock = new VmBlock(InplaceStr("arr_setup_cond"), module->nextBlockId++);
		VmBlock *bodyBlock = new VmBlock(InplaceStr("arr_setup_body"), module->nextBlockId++);
		VmBlock *exitBlock = new VmBlock(InplaceStr("arr_setup_exit"), module->nextBlockId++);

		CreateJump(module, conditionBlock);

		module->currentFunction->AddBlock(conditionBlock);
		module->currentBlock = conditionBlock;

		// Offset will move in element size steps, so it will reach the full size of the array
		assert(int(arrayType->length * arrayType->subType->size) == arrayType->length * arrayType->subType->size);

		// While offset is less than array size
		VmValue* condition = CreateCompareLess(module, CreateLoad(ctx, module, ctx.typeInt, offsetPtr), CreateConstantInt(int(arrayType->length * arrayType->subType->size)));

		CreateJumpNotZero(module, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		VmValue *offset = CreateLoad(ctx, module, ctx.typeInt, offsetPtr);

		CreateStore(ctx, module, arrayType->subType, CreateAdd(module, address, offset), initializer);
		CreateStore(ctx, module, ctx.typeInt, offsetPtr, CreateAdd(module, offset, CreateConstantInt(int(arrayType->subType->size))));

		CreateJump(module, conditionBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(expression))
	{
		for(ExprVariableDefinition *value = node->definitions.head; value; value = getType<ExprVariableDefinition>(value->next))
			CompileVm(ctx, module, value);

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
	{
		VmValue *address = CreateVariableAddress(node->variable);

		VmValue *value = CreateLoad(ctx, module, node->variable->type, address);

		value->comment = node->variable->name;

		return CheckType(ctx, expression, value);
	}
	else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
	{
		VmFunction *function = node->function->vmFunction;

		if(node->function->isPrototype)
			return CheckType(ctx, expression, function);
		
		// Store state
		unsigned nextBlockId = module->nextBlockId;
		unsigned nextInstructionId = module->nextInstructionId;
		VmFunction *currentFunction = module->currentFunction;
		VmBlock *currentBlock = module->currentBlock;

		// Switch to new function
		module->nextBlockId = 1;
		module->nextInstructionId = 1;

		module->currentFunction = function;

		VmBlock *block = new VmBlock(InplaceStr("start"), module->nextBlockId++);

		module->currentFunction->AddBlock(block);
		module->currentBlock = block;
		block->AddUse(function);

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		// Restore state
		module->nextBlockId = nextBlockId;
		module->nextInstructionId = nextInstructionId;
		module->currentFunction = currentFunction;
		module->currentBlock = currentBlock;

		return CheckType(ctx, expression, function);
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
	{
		for(ExprBase *instance = node->function->instances.head; instance; instance = instance->next)
			CompileVm(ctx, module, instance);

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
	{
		assert(node->function->vmFunction);

		return CheckType(ctx, expression, node->function->vmFunction);
	}
	else if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
	{
		VmValue *function = CompileVm(ctx, module, node->function);

		assert(module->currentBlock);

		VmInstruction *inst = new VmInstruction(GetVmType(ctx, node->type), VM_INST_CALL, module->nextInstructionId++);

		unsigned argCount = 1;

		for(ExprBase *value = node->arguments.head; value; value = value->next)
			argCount++;

		inst->arguments.reserve(argCount);

		inst->AddArgument(function);

		for(ExprBase *value = node->arguments.head; value; value = value->next)
		{
			VmValue *argument = CompileVm(ctx, module, value);

			assert(argument->type != VmType::Void);

			inst->AddArgument(argument);
		}

		inst->hasSideEffects = HasSideEffects(inst->cmd);
		inst->hasMemoryAccess = HasMemoryAccess(inst->cmd);

		module->currentBlock->AddInstruction(inst);

		return CheckType(ctx, expression, inst);
	}
	else if(ExprAliasDefinition *node = getType<ExprAliasDefinition>(expression))
	{
		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprGenericClassPrototype *node = getType<ExprGenericClassPrototype>(expression))
	{
		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
	{
		for(ExprBase *value = node->functions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprIfElse *node = getType<ExprIfElse>(expression))
	{
		VmValue* condition = CompileVm(ctx, module, node->condition);

		VmBlock *trueBlock = new VmBlock(InplaceStr("if_true"), module->nextBlockId++);
		VmBlock *falseBlock = new VmBlock(InplaceStr("if_false"), module->nextBlockId++);
		VmBlock *exitBlock = new VmBlock(InplaceStr("if_exit"), module->nextBlockId++);

		if(node->falseBlock)
			CreateJumpNotZero(module, condition, trueBlock, falseBlock);
		else
			CreateJumpNotZero(module, condition, trueBlock, exitBlock);

		module->currentFunction->AddBlock(trueBlock);
		module->currentBlock = trueBlock;

		CompileVm(ctx, module, node->trueBlock);

		CreateJump(module, exitBlock);

		if(node->falseBlock)
		{
			module->currentFunction->AddBlock(falseBlock);
			module->currentBlock = falseBlock;

			CompileVm(ctx, module, node->falseBlock);

			CreateJump(module, exitBlock);
		}

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprFor *node = getType<ExprFor>(expression))
	{
		CompileVm(ctx, module, node->initializer);

		VmBlock *conditionBlock = new VmBlock(InplaceStr("for_cond"), module->nextBlockId++);
		VmBlock *bodyBlock = new VmBlock(InplaceStr("for_body"), module->nextBlockId++);
		VmBlock *iterationBlock = new VmBlock(InplaceStr("for_iter"), module->nextBlockId++);
		VmBlock *exitBlock = new VmBlock(InplaceStr("for_exit"), module->nextBlockId++);

		//currLoopIDs.push_back(LoopInfo(exitBlock, iterationBlock));

		CreateJump(module, conditionBlock);

		module->currentFunction->AddBlock(conditionBlock);
		module->currentBlock = conditionBlock;

		VmValue* condition = CompileVm(ctx, module, node->condition);

		CreateJumpNotZero(module, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		CompileVm(ctx, module, node->body);

		CreateJump(module, iterationBlock);

		module->currentFunction->AddBlock(iterationBlock);
		module->currentBlock = iterationBlock;

		CompileVm(ctx, module, node->increment);

		CreateJump(module, conditionBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		//currLoopIDs.pop_back();

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprWhile *node = getType<ExprWhile>(expression))
	{
		VmBlock *conditionBlock = new VmBlock(InplaceStr("while_cond"), module->nextBlockId++);
		VmBlock *bodyBlock = new VmBlock(InplaceStr("while_body"), module->nextBlockId++);
		VmBlock *exitBlock = new VmBlock(InplaceStr("while_exit"), module->nextBlockId++);

		//currLoopIDs.push_back(LoopInfo(exitBlock, conditionBlock));

		CreateJump(module, conditionBlock);

		module->currentFunction->AddBlock(conditionBlock);
		module->currentBlock = conditionBlock;

		VmValue* condition = CompileVm(ctx, module, node->condition);

		CreateJumpNotZero(module, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		CompileVm(ctx, module, node->body);

		CreateJump(module, conditionBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		//currLoopIDs.pop_back();

		return CheckType(ctx, expression, new VmVoid());
	}
	else if(ExprDoWhile *node = getType<ExprDoWhile>(expression))
	{
		VmBlock *bodyBlock = new VmBlock(InplaceStr("do_body"), module->nextBlockId++);
		VmBlock *condBlock = new VmBlock(InplaceStr("do_cond"), module->nextBlockId++);
		VmBlock *exitBlock = new VmBlock(InplaceStr("do_exit"), module->nextBlockId++);

		CreateJump(module, bodyBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		//currLoopIDs.push_back(LoopInfo(exitBlock, condBlock));

		CompileVm(ctx, module, node->body);

		CreateJump(module, condBlock);

		module->currentFunction->AddBlock(condBlock);
		module->currentBlock = condBlock;

		VmValue* condition = CompileVm(ctx, module, node->condition);

		CreateJumpNotZero(module, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		//currLoopIDs.pop_back();
	}
	else if(ExprBlock *node = getType<ExprBlock>(expression))
	{
		for(ExprBase *value = node->expressions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		return CheckType(ctx, expression, new VmVoid());
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
		VmModule *module = new VmModule();

		// Generate global function
		VmFunction *global = new VmFunction(VmType::Void, NULL, VmType::Void);

		// Generate type indexes
		for(unsigned i = 0; i < ctx.types.size(); i++)
		{
			TypeBase *type = ctx.types[i];

			type->typeIndex = i;
		}

		// Generate VmFunction object for each function
		for(unsigned i = 0; i < ctx.functions.size(); i++)
		{
			FunctionData *function = ctx.functions[i];

			if(function->type->isGeneric)
				continue;

			if(function->vmFunction)
				continue;

			VmFunction *vmFunction = new VmFunction(GetVmType(ctx, function->type), function, GetVmType(ctx, function->type->returnType));

			function->vmFunction = vmFunction;

			if(FunctionData *implementation = function->implementation)
				implementation->vmFunction = vmFunction;

			module->functions.push_back(vmFunction);
		}

		// Setup global function
		module->currentFunction = global;

		VmBlock *block = new VmBlock(InplaceStr("start"), module->nextBlockId++);

		global->AddBlock(block);
		module->currentBlock = block;
		block->AddUse(global);

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		module->functions.push_back(global);

		return module;
	}

	return NULL;
}

void RunPeepholeOptimizations(VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunPeepholeOptimizations(module, curr);
			curr = next;
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		VmInstruction *curr = block->firstInstruction;

		while(curr)
		{
			VmInstruction *next = curr->nextSibling;
			RunPeepholeOptimizations(module, curr);
			curr = next;
		}
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		switch(inst->cmd)
		{
		case VM_INST_ADD:
			if(IsConstantZero(inst->arguments[0])) // 0 + x, all types
				ReplaceValueUsersWith(inst, inst->arguments[1], &module->peepholeOptimizations);
			else if(IsConstantZero(inst->arguments[1])) // x + 0, all types
				ReplaceValueUsersWith(inst, inst->arguments[0], &module->peepholeOptimizations);
			break;
		case VM_INST_SUB:
			if(DoesConstantIntegerMatch(inst->arguments[0], 0)) // 0 - x, integer types
				ChangeInstructionTo(inst, VM_INST_NEG, inst->arguments[1], NULL, NULL, NULL, &module->peepholeOptimizations);
			else if(IsConstantZero(inst->arguments[1])) // x - 0, all types
				ReplaceValueUsersWith(inst, inst->arguments[0], &module->peepholeOptimizations);
			break;
		case VM_INST_MUL:
			if(IsConstantZero(inst->arguments[0]) || IsConstantZero(inst->arguments[1])) // 0 * x or x * 0, all types
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(inst, CreateConstantInt(0), &module->peepholeOptimizations);
				else if(inst->type == VmType::Double)
					ReplaceValueUsersWith(inst, CreateConstantDouble(0), &module->peepholeOptimizations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(inst, CreateConstantLong(0), &module->peepholeOptimizations);
			}
			else if(IsConstantOne(inst->arguments[0])) // 1 * x, all types
			{
				ReplaceValueUsersWith(inst, inst->arguments[1], &module->peepholeOptimizations);
			}
			else if(IsConstantOne(inst->arguments[1])) // x * 1, all types
			{
				ReplaceValueUsersWith(inst, inst->arguments[0], &module->peepholeOptimizations);
			}
			break;
		case VM_INST_INDEX_UNSIZED:
			// Try to replace unsized array index with an array index if the type[] is a construct expression
			if(VmInstruction *objectConstruct = getType<VmInstruction>(inst->arguments[1]))
			{
				if(objectConstruct->cmd == VM_INST_CONSTRUCT && isType<VmConstant>(objectConstruct->arguments[1]))
					ChangeInstructionTo(inst, VM_INST_INDEX, objectConstruct->arguments[1], inst->arguments[0], objectConstruct->arguments[0], inst->arguments[2], &module->peepholeOptimizations);
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
							ReplaceValueUsersWith(inst, objectConstruct->arguments[1], &module->peepholeOptimizations);
					}
				}
			}
			break;
		}
	}
}

void RunConstantPropagation(VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunConstantPropagation(module, curr);
			curr = next;
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		VmInstruction *curr = block->firstInstruction;

		while(curr)
		{
			VmInstruction *next = curr->nextSibling;
			RunConstantPropagation(module, curr);
			curr = next;
		}
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		if(inst->type != VmType::Int && inst->type != VmType::Double && inst->type != VmType::Long && inst->type != VmType::Pointer)
			return;

		SmallArray<VmConstant*, 32> consts;

		for(unsigned i = 0; i < inst->arguments.size(); i++)
		{
			VmConstant *constant = getType<VmConstant>(inst->arguments[i]);

			if(!constant)
				return;

			consts.push_back(constant);
		}

		switch(inst->cmd)
		{
		case VM_INST_DOUBLE_TO_INT:
			ReplaceValueUsersWith(inst, CreateConstantInt(int(consts[0]->dValue)), &module->constantPropagations);
			break;
		case VM_INST_DOUBLE_TO_LONG:
			ReplaceValueUsersWith(inst, CreateConstantLong((long long)(consts[0]->dValue)), &module->constantPropagations);
			break;
		case VM_INST_INT_TO_DOUBLE:
			ReplaceValueUsersWith(inst, CreateConstantDouble(double(consts[0]->iValue)), &module->constantPropagations);
			break;
		case VM_INST_LONG_TO_DOUBLE:
			ReplaceValueUsersWith(inst, CreateConstantDouble(double(consts[0]->lValue)), &module->constantPropagations);
			break;
		case VM_INST_INT_TO_LONG:
			ReplaceValueUsersWith(inst, CreateConstantLong((long long)(consts[0]->iValue)), &module->constantPropagations);
			break;
		case VM_INST_LONG_TO_INT:
			ReplaceValueUsersWith(inst, CreateConstantInt(int(consts[0]->lValue)), &module->constantPropagations);
			break;
		case VM_INST_ADD:
			if(inst->type == VmType::Pointer)
			{
				// Both arguments can't be a frame offset
				assert(!(consts[0]->isFrameOffset && consts[1]->isFrameOffset));

				ReplaceValueUsersWith(inst, CreateConstantPointer(consts[0]->iValue + consts[1]->iValue, consts[0]->isFrameOffset || consts[1]->isFrameOffset), &module->constantPropagations);
			}
			else
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue + consts[1]->iValue), &module->constantPropagations);
				else if(inst->type == VmType::Double)
					ReplaceValueUsersWith(inst, CreateConstantDouble(consts[0]->dValue + consts[1]->dValue), &module->constantPropagations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue + consts[1]->lValue), &module->constantPropagations);
			}
			break;
		case VM_INST_SUB:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue - consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantDouble(consts[0]->dValue - consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue - consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_MUL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue * consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantDouble(consts[0]->dValue * consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue * consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_DIV:
			if(!IsConstantZero(consts[1]))
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue / consts[1]->iValue), &module->constantPropagations);
				else if(inst->type == VmType::Double)
					ReplaceValueUsersWith(inst, CreateConstantDouble(consts[0]->dValue / consts[1]->dValue), &module->constantPropagations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue / consts[1]->lValue), &module->constantPropagations);
			}
			break;
		case VM_INST_MOD:
			if(!IsConstantZero(consts[1]))
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue % consts[1]->iValue), &module->constantPropagations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue % consts[1]->lValue), &module->constantPropagations);
			}
			break;
		case VM_INST_LESS:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue < consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->dValue < consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue < consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_GREATER:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue > consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->dValue > consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue > consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LESS_EQUAL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue <= consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->dValue <= consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue <= consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_GREATER_EQUAL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue >= consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->dValue >= consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue >= consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_EQUAL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue == consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->dValue == consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue == consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_NOT_EQUAL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue != consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->dValue != consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue != consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_SHL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue << consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue << consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_SHR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue >> consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue >> consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_AND:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue & consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue & consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_OR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue | consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue | consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_XOR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue ^ consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(consts[0]->lValue ^ consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LOG_AND:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue && consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue && consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LOG_OR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->iValue || consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt(consts[0]->lValue || consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LOG_XOR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt((consts[0]->iValue != 0) != (consts[1]->iValue != 0)), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantInt((consts[0]->lValue != 0) != (consts[1]->lValue != 0)), &module->constantPropagations);
			break;
		case VM_INST_NEG:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(-consts[0]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(inst, CreateConstantDouble(-consts[0]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(-consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_NOT:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(~consts[0]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(~consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LOG_NOT:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(inst, CreateConstantInt(!consts[0]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(inst, CreateConstantLong(!consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_INDEX:
			{
				unsigned arrayLength = consts[0]->iValue;
				unsigned elementSize = consts[1]->iValue;

				unsigned ptr = consts[2]->iValue;
				unsigned index = consts[3]->iValue;

				if(index < arrayLength)
					ReplaceValueUsersWith(inst, CreateConstantPointer(ptr + elementSize * index, consts[2]->isFrameOffset), &module->constantPropagations);
			}
			break;
		}
	}
}

void RunDeadCodeElimiation(VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunDeadCodeElimiation(module, curr);
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
				RunDeadCodeElimiation(module, curr);
				curr = next;
			}
		}
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		if(inst->users.empty() && !inst->hasSideEffects)
		{
			module->deadCodeEliminations++;

			inst->parent->RemoveInstruction(inst);
		}
		else if(inst->cmd == VM_INST_JUMP_Z || inst->cmd == VM_INST_JUMP_NZ)
		{
			if(VmConstant *condition = getType<VmConstant>(inst->arguments[0]))
			{
				if(inst->cmd == VM_INST_JUMP_Z)
					ChangeInstructionTo(inst, VM_INST_JUMP, condition->iValue == 0 ? inst->arguments[1] : inst->arguments[2], NULL, NULL, NULL, &module->deadCodeEliminations);
				else
					ChangeInstructionTo(inst, VM_INST_JUMP, condition->iValue == 0 ? inst->arguments[2] : inst->arguments[1], NULL, NULL, NULL, &module->deadCodeEliminations);
			}
		}
	}
}

void RunControlFlowOptimization(VmModule *module, VmValue *value)
{
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
		}

		for(VmBlock *curr = function->firstBlock; curr;)
		{
			VmBlock *next = curr->nextSibling;

			if(curr->users.empty())
			{
				// Remove unused blocks
				function->RemoveBlock(curr);

				module->controlFlowSimplifications++;
			}
			else if(curr->firstInstruction && curr->firstInstruction == curr->lastInstruction && curr->firstInstruction->cmd == VM_INST_JUMP)
			{
				// Remove blocks that only contain an unconditional branch to some other block
				VmBlock *target = getType<VmBlock>(curr->firstInstruction->arguments[0]);

				assert(target);

				ReplaceValueUsersWith(curr, target, &module->controlFlowSimplifications);
			}

			curr = next;
		}
	}
}

void RunLoadStorePropagation(VmModule *module, VmValue *value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunLoadStorePropagation(module, curr);
			curr = next;
		}
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
					ReplaceValueUsersWith(curr, value, &module->loadStorePropagations);
				else
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
			case VM_INST_YIELD:
			case VM_INST_CLOSE_UPVALUES:
				ClearLoadStoreInfo(module);
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

				if(prev && (prev->cmd >= VM_INST_STORE_BYTE && prev->cmd <= VM_INST_STORE_STRUCT) && GetAccessSize(prev) == GetAccessSize(curr) && prev->arguments[0] == curr->arguments[0])
					ReplaceValueUsersWith(curr, prev->arguments[1], &module->loadStorePropagations);
			}

			curr = next;
		}
	}
}

void RunCommonSubexpressionElimination(VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunCommonSubexpressionElimination(module, curr);
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
						ReplaceValueUsersWith(curr, prev, &module->commonSubexprEliminations);
						break;
					}
				}

				prev = prev->nextSibling;
			}

			curr = next;
		}
	}
}

void RunOptimizationPass(VmModule *module, VmOptimizationType type)
{
	for(VmFunction *value = module->functions.head; value; value = value->next)
	{
		switch(type)
		{
		case VM_OPT_PEEPHOLE:
			RunPeepholeOptimizations(module, value);
			break;
		case VM_OPT_CONSTANT_PROPAGATION:
			RunConstantPropagation(module, value);
			break;
		case VM_OPT_DEAD_CODE_ELIMINATION:
			RunDeadCodeElimiation(module, value);
			break;
		case VM_OPT_CONTROL_FLOW_SIPLIFICATION:
			RunControlFlowOptimization(module, value);
			break;
		case VM_OPT_LOAD_STORE_PROPAGATION:
			RunLoadStorePropagation(module, value);
			break;
		case VM_OPT_COMMON_SUBEXPRESSION_ELIMINATION:
			RunCommonSubexpressionElimination(module, value);
			break;
		}
	}
}
