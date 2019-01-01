#include "InstructionTreeVmCommon.h"

#include "TypeTree.h"
#include "InstructionTreeVm.h"

#define allocate(T) new ((T*)allocator->alloc(sizeof(T))) T

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
	return allocate(VmConstant)(allocator, VmType::Void, NULL);
}

VmConstant* CreateConstantInt(Allocator *allocator, SynBase *source, int value)
{
	VmConstant *result = allocate(VmConstant)(allocator, VmType::Int, source);

	result->iValue = value;

	return result;
}

VmConstant* CreateConstantDouble(Allocator *allocator, SynBase *source, double value)
{
	VmConstant *result = allocate(VmConstant)(allocator, VmType::Double, source);

	result->dValue = value;

	return result;
}

VmConstant* CreateConstantLong(Allocator *allocator, SynBase *source, long long value)
{
	VmConstant *result = allocate(VmConstant)(allocator, VmType::Long, source);

	result->lValue = value;

	return result;
}

VmConstant* CreateConstantPointer(Allocator *allocator, SynBase *source, int offset, VariableData *container, TypeBase *structType, bool trackUsers)
{
	if(trackUsers && container)
	{
		for(unsigned i = 0; i < container->users.size(); i++)
		{
			if(container->users[i]->iValue == offset)
				return container->users[i];
		}
	}

	VmConstant *result = allocate(VmConstant)(allocator, VmType::Pointer(structType), source);

	result->iValue = offset;
	result->container = container;

	if(trackUsers && container)
		container->users.push_back(result);

	return result;
}

VmConstant* CreateConstantStruct(Allocator *allocator, SynBase *source, char *value, int size, TypeBase *structType)
{
	assert(size % 4 == 0);

	VmConstant *result = allocate(VmConstant)(allocator, VmType::Struct(size, structType), source);

	result->sValue = value;

	return result;
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
		return inst->arguments[1]->type.size;
	}

	return 0;
}

bool HasAddressTaken(VariableData *container)
{
	for(unsigned i = 0; i < container->users.size(); i++)
	{
		VmConstant *user = container->users[i];

		for(unsigned i = 0; i < user->users.size(); i++)
		{
			if(VmInstruction *inst = getType<VmInstruction>(user->users[i]))
			{
				bool simpleUse = false;

				if(inst->cmd >= VM_INST_LOAD_BYTE && inst->cmd <= VM_INST_LOAD_STRUCT)
					simpleUse = true;
				else if(inst->cmd >= VM_INST_STORE_BYTE && inst->cmd <= VM_INST_STORE_STRUCT && inst->arguments[0] == user)
					simpleUse = true;
				else
					simpleUse = false;

				if(!simpleUse)
					return true;
			}
			else
			{
				assert(!"invalid constant use");
			}
		}
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
	case VM_INST_LOG_XOR:
		return "lxor";
	case VM_INST_NEG:
		return "neg";
	case VM_INST_BIT_NOT:
		return "not";
	case VM_INST_LOG_NOT:
		return "lnot";
	case VM_INST_CREATE_CLOSURE:
		return "create_closure";
	case VM_INST_CLOSE_UPVALUES:
		return "close_upvalues";
	case VM_INST_CONVERT_POINTER:
		return "convert_pointer";
	case VM_INST_CHECKED_RETURN:
		return "checked_return";
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
	default:
		assert(!"unknown instruction");
	}

	return "unknown";
}
