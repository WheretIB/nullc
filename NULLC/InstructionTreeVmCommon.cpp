#include "InstructionTreeVmCommon.h"

#include "TypeTree.h"
#include "InstructionTreeVm.h"

#define allocate(T) new ((T*)allocator->alloc(sizeof(T))) T

bool IsGlobalScope(ScopeData *scope)
{
	if(!scope)
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
	return allocate(VmConstant)(allocator, VmType::Void);
}

VmConstant* CreateConstantInt(Allocator *allocator, int value)
{
	VmConstant *result = allocate(VmConstant)(allocator, VmType::Int);

	result->iValue = value;

	return result;
}

VmConstant* CreateConstantDouble(Allocator *allocator, double value)
{
	VmConstant *result = allocate(VmConstant)(allocator, VmType::Double);

	result->dValue = value;

	return result;
}

VmConstant* CreateConstantLong(Allocator *allocator, long long value)
{
	VmConstant *result = allocate(VmConstant)(allocator, VmType::Long);

	result->lValue = value;

	return result;
}

VmConstant* CreateConstantPointer(Allocator *allocator, int offset, VariableData *container, TypeBase *structType, bool trackUsers)
{
	if(trackUsers && container)
	{
		for(unsigned i = 0; i < container->users.size(); i++)
		{
			if(container->users[i]->iValue == offset)
				return container->users[i];
		}
	}

	VmConstant *result = allocate(VmConstant)(allocator, VmType::Pointer(structType));

	result->iValue = offset;
	result->container = container;

	if(trackUsers && container)
		container->users.push_back(result);

	return result;
}

VmConstant* CreateConstantStruct(Allocator *allocator, char *value, int size, TypeBase *structType)
{
	assert(size % 4 == 0);

	VmConstant *result = allocate(VmConstant)(allocator, VmType::Struct(size, structType));

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
