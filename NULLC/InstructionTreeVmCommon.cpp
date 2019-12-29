#include "InstructionTreeVmCommon.h"

#include "TypeTree.h"
#include "InstructionTreeVm.h"
#include "ExpressionTree.h"

namespace
{
	template<typename T>
	T* get(Allocator *allocator)
	{
		return (T*)allocator->alloc(sizeof(T));
	}
}

bool IsGlobalScope(ScopeData *scope)
{
	if(!scope)
		return false;

	if(scope->type == SCOPE_TEMPORARY)
		return false;

	while(scope->scope)
	{
		if(scope->ownerFunction)
			return false;

		if(scope->ownerType)
			return false;

		scope = scope->scope;
	}

	return true;
}

bool IsMemberScope(ScopeData *scope)
{
	return scope->ownerType != NULL;
}

bool IsLocalScope(ScopeData *scope)
{
	if(!scope)
		return false;

	if(scope->type == SCOPE_TEMPORARY)
		return false;

	// Not a global scope if there is an enclosing function or a type
	while(scope)
	{
		if(scope->ownerFunction || scope->ownerType)
			return true;

		scope = scope->scope;
	}

	return false;
}

VmConstant* CreateConstantVoid(Allocator *allocator)
{
	return new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Void, NULL);
}

VmConstant* CreateConstantInt(Allocator *allocator, SynBase *source, int value)
{
	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Int, source);

	result->iValue = value;

	return result;
}

VmConstant* CreateConstantDouble(Allocator *allocator, SynBase *source, double value)
{
	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Double, source);

	result->dValue = value;

	return result;
}

VmConstant* CreateConstantLong(Allocator *allocator, SynBase *source, long long value)
{
	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Long, source);

	result->lValue = value;

	return result;
}

VmConstant* CreateConstantPointer(Allocator *allocator, SynBase *source, int offset, VariableData *container, TypeBase *structType, bool trackUsers)
{
	if(trackUsers && container)
	{
		if(VmConstant **cached = container->offsetUsers.find(offset + 1))
		{
			if (VmConstant *constant = *cached)
				return constant;
		}
	}

	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Pointer(structType), source);

	result->iValue = offset;
	result->container = container;

	if(trackUsers && container)
	{
		container->users.push_back(result);
		container->offsetUsers.insert(offset + 1, result);
	}

	return result;
}

VmConstant* CreateConstantStruct(Allocator *allocator, SynBase *source, char *value, int size, TypeBase *structType)
{
	assert(size % 4 == 0);

	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Struct(size, structType), source);

	result->sValue = value;

	return result;
}

VmConstant* CreateConstantBlock(Allocator *allocator, SynBase *source, VmBlock *block)
{
	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Block, source);

	result->bValue = block;

	return result;
}

VmConstant* CreateConstantFunction(Allocator *allocator, SynBase *source, VmFunction *function)
{
	VmConstant *result = new (get<VmConstant>(allocator)) VmConstant(allocator, VmType::Function, source);

	result->fValue = function;

	return result;
}

VmConstant* CreateConstantZero(Allocator *allocator, SynBase *source, VmType type)
{
	if(type == VmType::Int)
		return CreateConstantInt(allocator, source, 0);

	if(type == VmType::Double)
		return CreateConstantDouble(allocator, source, 0);

	if(type == VmType::Long)
		return CreateConstantLong(allocator, source, 0);

	assert(!"unknown type");
	return NULL;
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
		return inst->arguments[2]->type.size;
	case VM_INST_MEM_COPY:
		if(VmConstant *size = getType<VmConstant>(inst->arguments[4]))
			return size->iValue;

		assert(!"invalid memcopy instruction");
		break;
	case VM_INST_ADD_LOAD:
	case VM_INST_SUB_LOAD:
	case VM_INST_MUL_LOAD:
	case VM_INST_DIV_LOAD:
	case VM_INST_POW_LOAD:
	case VM_INST_MOD_LOAD:
	case VM_INST_LESS_LOAD:
	case VM_INST_GREATER_LOAD:
	case VM_INST_LESS_EQUAL_LOAD:
	case VM_INST_GREATER_EQUAL_LOAD:
	case VM_INST_EQUAL_LOAD:
	case VM_INST_NOT_EQUAL_LOAD:
	case VM_INST_SHL_LOAD:
	case VM_INST_SHR_LOAD:
	case VM_INST_BIT_AND_LOAD:
	case VM_INST_BIT_OR_LOAD:
	case VM_INST_BIT_XOR_LOAD:
		if(VmConstant *loadInst = getType<VmConstant>(inst->arguments[3]))
		{
			switch(loadInst->iValue)
			{
			case VM_INST_LOAD_INT:
			case VM_INST_LOAD_FLOAT:
				return 4;
			case VM_INST_LOAD_DOUBLE:
			case VM_INST_LOAD_LONG:
				return 8;
			}
		}

		assert(!"unknown load type");
		break;
	default:
		break;
	}

	assert(!"unknown access instruction");
	return 0;
}

bool HasAddressTaken(VariableData *container)
{
	if(container->isVmRegSpill)
		return false;

	for(unsigned userPos = 0, userCount = container->users.count; userPos < userCount; userPos++)
	{
		VmConstant *user = container->users.data[userPos];

		if(user->hasKnownSimpleUse)
		{
			assert(!user->hasKnownNonSimpleUse);
			continue;
		}

		if(user->hasKnownNonSimpleUse)
			return true;

		for(unsigned i = 0, e = user->users.count; i < e; i++)
		{
			VmValue *value = user->users.data[i];

			if(VmInstruction *inst = value->typeID == VmInstruction::myTypeID ? static_cast<VmInstruction*>(value) : NULL)
			{
				bool simpleUse = false;

				if(inst->cmd >= VM_INST_LOAD_BYTE && inst->cmd <= VM_INST_LOAD_STRUCT)
				{
					simpleUse = true;
				}
				else if(inst->cmd >= VM_INST_STORE_BYTE && inst->cmd <= VM_INST_STORE_STRUCT && inst->arguments[0] == user)
				{
					simpleUse = true;
				}
				else if(inst->cmd == VM_INST_MEM_COPY && (inst->arguments[0] == user || inst->arguments[2] == user))
				{
					simpleUse = true;
				}
				else if(inst->cmd == VM_INST_RETURN || inst->cmd == VM_INST_CALL)
				{
					if(user->isReference)
						simpleUse = true;
					else
						simpleUse = false;
				}
				else
				{
					simpleUse = false;
				}

				if(!simpleUse)
				{
					user->hasKnownNonSimpleUse = true;

					return true;
				}
			}
			else
			{
				assert(!"invalid constant use");
			}
		}

		user->hasKnownSimpleUse = true;
	}

	return false;
}

const char* GetInstructionName(VmInstruction *inst)
{
	VmInstructionType cmd = inst->cmd;

	switch(cmd)
	{
	case VM_INST_LOAD_BYTE:
		return "loadb";
	case VM_INST_LOAD_SHORT:
		return "loadw";
	case VM_INST_LOAD_INT:
		return "load";
	case VM_INST_LOAD_FLOAT:
		return "loadf";
	case VM_INST_LOAD_DOUBLE:
		return "loadd";
	case VM_INST_LOAD_LONG:
		return "loadl";
	case VM_INST_LOAD_STRUCT:
		return "loads";
	case VM_INST_LOAD_IMMEDIATE:
		return "loadimm";
	case VM_INST_STORE_BYTE:
		return "storeb";
	case VM_INST_STORE_SHORT:
		return "storew";
	case VM_INST_STORE_INT:
		return "store";
	case VM_INST_STORE_FLOAT:
		return "storef";
	case VM_INST_STORE_DOUBLE:
		return "stored";
	case VM_INST_STORE_LONG:
		return "storel";
	case VM_INST_STORE_STRUCT:
		return "stores";
	case VM_INST_DOUBLE_TO_INT:
		return "dti";
	case VM_INST_DOUBLE_TO_LONG:
		return "dtl";
	case VM_INST_DOUBLE_TO_FLOAT:
		return "dtf";
	case VM_INST_INT_TO_DOUBLE:
		return "itd";
	case VM_INST_LONG_TO_DOUBLE:
		return "ltd";
	case VM_INST_INT_TO_LONG:
		return "itl";
	case VM_INST_LONG_TO_INT:
		return "lti";
	case VM_INST_INDEX:
		return "index";
	case VM_INST_INDEX_UNSIZED:
		return "indexu";
	case VM_INST_FUNCTION_ADDRESS:
		return "faddr";
	case VM_INST_TYPE_ID:
		return "typeid";
	case VM_INST_SET_RANGE:
		return "setrange";
	case VM_INST_MEM_COPY:
		return "memcopy";
	case VM_INST_JUMP:
		return "jmp";
	case VM_INST_JUMP_Z:
		return "jmpz";
	case VM_INST_JUMP_NZ:
		return "jmpnz";
	case VM_INST_CALL:
		return "call";
	case VM_INST_RETURN:
		return "ret";
	case VM_INST_YIELD:
		return "yield";
	case VM_INST_ADD:
		return "add";
	case VM_INST_SUB:
		return "sub";
	case VM_INST_MUL:
		return "mul";
	case VM_INST_DIV:
		return "div";
	case VM_INST_POW:
		return "pow";
	case VM_INST_MOD:
		return "mod";
	case VM_INST_LESS:
		return "lt";
	case VM_INST_GREATER:
		return "gt";
	case VM_INST_LESS_EQUAL:
		return "lte";
	case VM_INST_GREATER_EQUAL:
		return "gte";
	case VM_INST_EQUAL:
		return "eq";
	case VM_INST_NOT_EQUAL:
		return "neq";
	case VM_INST_SHL:
		return "shl";
	case VM_INST_SHR:
		return "shr";
	case VM_INST_BIT_AND:
		return "and";
	case VM_INST_BIT_OR:
		return "or";
	case VM_INST_BIT_XOR:
		return "xor";
	case VM_INST_ADD_LOAD:
		return "add_load";
	case VM_INST_SUB_LOAD:
		return "sub_load";
	case VM_INST_MUL_LOAD:
		return "mul_load";
	case VM_INST_DIV_LOAD:
		return "div_load";
	case VM_INST_POW_LOAD:
		return "pow_load";
	case VM_INST_MOD_LOAD:
		return "mod_load";
	case VM_INST_LESS_LOAD:
		return "lt_load";
	case VM_INST_GREATER_LOAD:
		return "gt_load";
	case VM_INST_LESS_EQUAL_LOAD:
		return "lte_load";
	case VM_INST_GREATER_EQUAL_LOAD:
		return "gte_load";
	case VM_INST_EQUAL_LOAD:
		return "eq_load";
	case VM_INST_NOT_EQUAL_LOAD:
		return "neq_load";
	case VM_INST_SHL_LOAD:
		return "shl_load";
	case VM_INST_SHR_LOAD:
		return "shr_load";
	case VM_INST_BIT_AND_LOAD:
		return "and_load";
	case VM_INST_BIT_OR_LOAD:
		return "or_load";
	case VM_INST_BIT_XOR_LOAD:
		return "xor_load";
	case VM_INST_NEG:
		return "neg";
	case VM_INST_BIT_NOT:
		return "not";
	case VM_INST_LOG_NOT:
		return "lnot";
	case VM_INST_CONVERT_POINTER:
		return "convert_pointer";
	case VM_INST_ABORT_NO_RETURN:
		return "abort_no_return";
	case VM_INST_CONSTRUCT:
		return "construct";
	case VM_INST_ARRAY:
		return "array";
	case VM_INST_EXTRACT:
		return "extract";
	case VM_INST_UNYIELD:
		return "unyield";
	case VM_INST_PHI:
		return "phi";
	case VM_INST_BITCAST:
		return "bitcast";
	case VM_INST_MOV:
		return "mov";
	case VM_INST_DEF:
		return "def";
	case VM_INST_PARALLEL_COPY:
		return "parallel_copy";
	default:
		assert(!"unknown instruction");
	}

	return "unknown";
}

VariableData* FindGlobalAt(ExpressionContext &exprCtx, unsigned offset)
{
	unsigned targetModuleIndex = offset >> 24;

	if(targetModuleIndex)
		offset = offset & 0xffffff;

	for(unsigned i = 0; i < exprCtx.variables.size(); i++)
	{
		VariableData *variable = exprCtx.variables[i];

		unsigned variableModuleIndex = variable->importModule ? variable->importModule->importIndex : 0;

		if(IsGlobalScope(variable->scope) && variableModuleIndex == targetModuleIndex && offset >= variable->offset && (offset < variable->offset + variable->type->size || variable->type->size == 0))
			return variable;
	}

	return NULL;
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
