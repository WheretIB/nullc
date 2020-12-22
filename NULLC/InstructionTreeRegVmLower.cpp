#include "InstructionTreeRegVmLower.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

namespace
{
	bool IsNonLocalValue(VmInstruction *inst)
	{
		if(inst->users.empty())
			return false;

		for(unsigned i = 0; i < inst->users.size(); i++)
		{
			VmInstruction *user = getType<VmInstruction>(inst->users[i]);

			if(inst->parent != user->parent)
				return true;
		}

		return false;
	}
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, RegVmLoweredInstruction* instruction)
{
	(void)ctx;

	assert(instruction);
	assert(instruction->parent == NULL);
	assert(instruction->prevSibling == NULL);
	assert(instruction->nextSibling == NULL);

	instruction->parent = this;

	if(instruction->argument && instruction->argument->container)
		instruction->argument->container->regVmUsers.push_back(instruction);

	if(!firstInstruction)
	{
		firstInstruction = lastInstruction = instruction;
	}
	else
	{
		lastInstruction->nextSibling = instruction;
		instruction->prevSibling = lastInstruction;

		lastInstruction = instruction;
	}

	for(unsigned i = 0; i < parent->killedRegisters.size(); i++)
		instruction->preKillRegisters.push_back(parent->killedRegisters[i]);
	parent->killedRegisters.clear();
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(ctx.allocator, location, code, 0, 0, 0, NULL));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(ctx.allocator, location, code, rA, rB, rC, NULL));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(ctx.allocator, location, code, rA, rB, rC, argument));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(ctx.allocator, location, code, rA, rB, rC, CreateConstantInt(ctx.allocator, NULL, argument)));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmBlock *argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(ctx.allocator, location, code, rA, rB, rC, argument ? CreateConstantBlock(ctx.allocator, NULL, argument) : NULL));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmFunction *argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(ctx.allocator, location, code, rA, rB, rC, argument ? CreateConstantFunction(ctx.allocator, NULL, argument) : NULL));
}

unsigned char RegVmLoweredFunction::GetRegister()
{
	unsigned char reg = 0;

	if(!freedRegisters.empty())
	{
		reg = freedRegisters.back();
		freedRegisters.pop_back();
	}
	else
	{
		// We start from rvrrCount register, so 0 means that we wrapped around from 255
		if(nextRegister == 0)
		{
			hasRegisterOverflow = true;
			return 255;
		}

		reg = nextRegister;
		nextRegister++;
	}

	assert(registerUsers[reg] == 0);
	registerUsers[reg]++;

	return reg;
}

void RegVmLoweredFunction::FreeRegister(unsigned char reg)
{
	assert(registerUsers[reg] != 0);
	registerUsers[reg]--;

	if(registerUsers[reg] == 0)
		delayedFreedRegisters.push_back(reg);
}

void RegVmLoweredFunction::CompleteUse(VmValue *value)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	assert(instruction);

	// Can't free instruction register if instruction has uses in multiple blocks, if the register gets used by a different instruction, they might intefere later
	if(IsNonLocalValue(instruction))
		return;

	// Phi instruction shares register ownership for instructions in different blocks, if this registers gets used by a different instruction, they might intefere later
	if(instruction->cmd == VM_INST_PHI)
		return;

	assert(instruction->regVmCompletedUsers < instruction->users.size());

	instruction->regVmCompletedUsers++;

	if(instruction->regVmCompletedUsers == instruction->users.size())
	{
		for(unsigned i = 0; i < instruction->regVmRegisters.size(); i++)
			FreeRegister(instruction->regVmRegisters[i]);
	}
}

unsigned char RegVmLoweredFunction::GetRegister(VmValue *value)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	CompleteUse(value);

	return instruction->regVmRegisters[0];
}

void RegVmLoweredFunction::GetRegisters(SmallArray<unsigned char, 32> &result, VmValue *value)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	CompleteUse(value);

	assert(!instruction->regVmRegisters.empty());

	for(unsigned i = 0; i < instruction->regVmRegisters.size(); i++)
		result.push_back(instruction->regVmRegisters[i]);
}

unsigned char RegVmLoweredFunction::AllocateRegister(VmValue *value, unsigned index, bool freeDelayed)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	if(freeDelayed)
		FreeDelayedRegisters(NULL);

	assert(instruction);
	assert(!instruction->users.empty());

	if(instruction->regVmAllocated)
		return instruction->regVmRegisters[index];

	assert(instruction->regVmRegisters.size() == index);

	instruction->regVmRegisters.push_back(GetRegister());

	return instruction->regVmRegisters.back();
}

unsigned char RegVmLoweredFunction::GetRegisterForConstant()
{
	unsigned char result = GetRegister();

	constantRegisters.push_back(result);

	return result;
}

void RegVmLoweredFunction::FreeConstantRegisters()
{
	for(unsigned i = 0; i < constantRegisters.size(); i++)
		FreeRegister(constantRegisters[i]);

	constantRegisters.clear();
}

void RegVmLoweredFunction::FreeDelayedRegisters(RegVmLoweredBlock *lowBlock)
{
	for(unsigned i = 0; i < delayedFreedRegisters.size(); i++)
	{
		if(lowBlock)
			lowBlock->lastInstruction->postKillRegisters.push_back(delayedFreedRegisters[i]);
		else
			killedRegisters.push_back(delayedFreedRegisters[i]);

		assert(!freedRegisters.contains(delayedFreedRegisters[i]));
		freedRegisters.push_back(delayedFreedRegisters[i]);
	}

	delayedFreedRegisters.clear();
}

bool RegVmLoweredFunction::TransferRegisterTo(VmValue *value, unsigned char reg)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	assert(instruction);

	if(IsNonLocalValue(instruction))
		return false;

	for(unsigned i = 0; i < constantRegisters.size(); i++)
	{
		if(constantRegisters[i] == reg)
		{
			assert(registerUsers[reg] == 1);

			constantRegisters[i] = constantRegisters.back();
			constantRegisters.pop_back();

			instruction->regVmRegisters.push_back(reg);

			return true;
		}
	}

	for(unsigned i = 0; i < delayedFreedRegisters.size(); i++)
	{
		if(delayedFreedRegisters[i] == reg)
		{
			assert(registerUsers[reg] == 0);

			delayedFreedRegisters[i] = delayedFreedRegisters.back();
			delayedFreedRegisters.pop_back();

			instruction->regVmRegisters.push_back(reg);

			registerUsers[reg]++;

			return true;
		}
	}

	for(unsigned i = 0; i < freedRegisters.size(); i++)
	{
		if(freedRegisters[i] == reg)
		{
			assert(registerUsers[reg] == 0);

			freedRegisters[i] = freedRegisters.back();
			freedRegisters.pop_back();

			instruction->regVmRegisters.push_back(reg);

			registerUsers[reg]++;

			return true;
		}
	}

	return false;
}

unsigned RegVmLoweredModule::FindConstant(unsigned value)
{
	for(unsigned i = 0; i < constants.size(); i++)
	{
		if(constants[i] == value)
			return i + 1;
	}

	return 0;
}

unsigned RegVmLoweredModule::FindConstant(unsigned value1, unsigned value2)
{
	for(unsigned i = 0; i + 1 < constants.size(); i += 2)
	{
		if(constants[i] == value1 && constants[i + 1] == value2)
			return i + 1;
	}

	return 0;
}

unsigned TryLowerConstantToMemory(RegVmLoweredBlock *lowBlock, VmValue *value);

void LowerConstantIntoBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, SmallArray<unsigned char, 32> &result, VmValue *value)
{
	VmConstant *constant = getType<VmConstant>(value);

	assert(constant);

	if(constant->type == VmType::Int)
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		lowBlock->AddInstruction(ctx, constant->source, rviLoadImm, targetReg, 0, 0, constant->iValue);

		result.push_back(targetReg);
	}
	else if(constant->type == VmType::Double)
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

		assert(constantIndex);

		lowBlock->AddInstruction(ctx, constant->source, rviLoadDouble, targetReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));

		result.push_back(targetReg);
	}
	else if(constant->type == VmType::Long)
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

		assert(constantIndex);

		lowBlock->AddInstruction(ctx, constant->source, rviLoadLong, targetReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));

		result.push_back(targetReg);
	}
	else if(constant->type.type == VM_TYPE_POINTER)
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		if(!constant->container)
		{
			assert(constant->iValue == 0);

			if(NULLC_PTR_SIZE == 8)
			{
				unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

				assert(constantIndex);

				lowBlock->AddInstruction(ctx, constant->source, rviLoadLong, targetReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
			}
			else
			{
				lowBlock->AddInstruction(ctx, constant->source, rviLoadImm, targetReg, 0, 0, 0u);
			}
		}
		else
		{
			lowBlock->AddInstruction(ctx, constant->source, rviGetAddr, targetReg, 0, IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals, constant);
		}

		result.push_back(targetReg);
	}
	else if(constant->type.type == VM_TYPE_STRUCT)
	{
		assert(constant->type.size % 4 == 0);

		unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

		assert(constantIndex);

		for(unsigned i = 0; i < constant->type.size / 4; i += 2)
		{
			unsigned char targetReg = lowFunction->GetRegisterForConstant();
			result.push_back(targetReg);

			if(constant->type.size / 4 - i >= 2)
				lowBlock->AddInstruction(ctx, constant->source, rviLoadLong, targetReg, 0, rvrrConstants, (constantIndex - 1 + i) * sizeof(unsigned));
			else
				lowBlock->AddInstruction(ctx, constant->source, rviLoadDword, targetReg, 0, rvrrConstants, (constantIndex - 1 + i) * sizeof(unsigned));
		}
	}
	else
	{
		assert(!"unknown type");
	}
}

bool TryLowerConstantPushIntoBlock(RegVmLoweredBlock *lowBlock, VmValue *value)
{
	if(VmConstant *constant = getType<VmConstant>(value))
	{
		RegVmLoweredModule *lowModule = lowBlock->parent->parent;

		if(constant->isReference)
		{
			lowModule->constants.push_back(rvmiPushMem);
			lowModule->constants.push_back(IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals);
			lowModule->constants.push_back(constant->container->offset);
			lowModule->constants.push_back(int(constant->type.size));

			return true;
		}

		if(constant->type == VmType::Int)
		{
			lowModule->constants.push_back(rvmiPushImm);
			lowModule->constants.push_back(constant->iValue);

			return true;
		}
		else if(constant->type == VmType::Double)
		{
			unsigned data[2];
			memcpy(data, &constant->dValue, 8);

			lowModule->constants.push_back(rvmiPushImm);
			lowModule->constants.push_back(data[0]);

			lowModule->constants.push_back(rvmiPushImm);
			lowModule->constants.push_back(data[1]);

			return true;
		}
		else if(constant->type == VmType::Long)
		{
			unsigned data[2];
			memcpy(data, &constant->lValue, 8);

			if(data[1] == 0)
			{
				lowModule->constants.push_back(rvmiPushImmq);
				lowModule->constants.push_back(data[0]);
			}
			else
			{
				lowModule->constants.push_back(rvmiPushImm);
				lowModule->constants.push_back(data[0]);

				lowModule->constants.push_back(rvmiPushImm);
				lowModule->constants.push_back(data[1]);
			}

			return true;
		}
		else if(constant->type.type == VM_TYPE_POINTER)
		{
			if(!constant->container)
			{
				assert(constant->iValue == 0);

				if(NULLC_PTR_SIZE == 8)
				{
					lowModule->constants.push_back(rvmiPushImmq);
					lowModule->constants.push_back(0);
				}
				else
				{
					lowModule->constants.push_back(rvmiPushImm);
					lowModule->constants.push_back(0);
				}

				return true;
			}
		}
		else if(constant->type.type == VM_TYPE_STRUCT)
		{
			assert(constant->type.size % 4 == 0);

			for(unsigned i = 0; i < constant->type.size / 4; i++)
			{
				unsigned elementValue;
				memcpy(&elementValue, constant->sValue + i * 4, 4);

				lowModule->constants.push_back(rvmiPushImm);
				lowModule->constants.push_back(elementValue);
			}

			return true;
		}
	}

	return false;
}

unsigned TryLowerConstantToMemory(RegVmLoweredBlock *lowBlock, VmValue *value)
{
	if(VmConstant *constant = getType<VmConstant>(value))
	{
		RegVmLoweredModule *lowModule = lowBlock->parent->parent;

		if(constant->type == VmType::Int || (constant->type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4 && !constant->container))
		{
			if(constant->type.type == VM_TYPE_POINTER)
				assert(constant->iValue == 0);

			if(unsigned index = lowModule->FindConstant(constant->iValue))
				return index;

			unsigned index = lowModule->constants.size() + 1;

			lowModule->constants.push_back(constant->iValue);

			return index;
		}
		else if(constant->type == VmType::Double)
		{
			unsigned data[2];
			memcpy(data, &constant->dValue, 8);

			if(unsigned index = lowModule->FindConstant(data[0], data[1]))
				return index;

			if(lowModule->constants.size() % 2 != 0)
				lowModule->constants.push_back(0);

			unsigned index = lowModule->constants.size() + 1;

			lowModule->constants.push_back(data[0]);
			lowModule->constants.push_back(data[1]);

			return index;
		}
		else if(constant->type == VmType::Long || (constant->type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8 && !constant->container))
		{
			if(constant->type.type == VM_TYPE_POINTER)
				assert(constant->iValue == 0);

			unsigned data[2];
			memcpy(data, &constant->lValue, 8);

			if(unsigned index = lowModule->FindConstant(data[0], data[1]))
				return index;

			if(lowModule->constants.size() % 2 != 0)
				lowModule->constants.push_back(0);

			unsigned index = lowModule->constants.size() + 1;

			lowModule->constants.push_back(data[0]);
			lowModule->constants.push_back(data[1]);

			return index;
		}
		else if(constant->type.type == VM_TYPE_STRUCT)
		{
			unsigned index = lowModule->constants.size() + 1;

			assert(constant->type.size % 4 == 0);

			for(unsigned i = 0; i < constant->type.size / 4; i++)
			{
				unsigned elementValue;
				memcpy(&elementValue, constant->sValue + i * 4, 4);
				lowModule->constants.push_back(elementValue);
			}

			return index;
		}
	}

	return 0;
}

unsigned char GetArgumentRegister(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmValue *value)
{
	if(isType<VmConstant>(value))
	{
		SmallArray<unsigned char, 32> result(ctx.allocator);

		LowerConstantIntoBlock(ctx, lowFunction, lowBlock, result, value);

		assert(result.size() == 1);

		return result.back();
	}

	if(VmFunction *function = getType<VmFunction>(value))
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		lowBlock->AddInstruction(ctx, function->source, rviFuncAddr, targetReg, 0, 0, function->function->functionIndex);

		return targetReg;
	}

	return lowFunction->GetRegister(value);
}

void GetArgumentRegisters(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, SmallArray<unsigned char, 32> &result, VmValue *value)
{
	if(isType<VmConstant>(value))
	{
		LowerConstantIntoBlock(ctx, lowFunction, lowBlock, result, value);

		return;
	}

	if(VmFunction *function = getType<VmFunction>(value))
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		lowBlock->AddInstruction(ctx, function->source, rviFuncAddr, targetReg, 0, 0, function->function->functionIndex);

		result.push_back(targetReg);

		return;
	}

	lowFunction->GetRegisters(result, value);
}

VmConstant* GetAddressInContainer(VmValue *address)
{
	if(VmConstant *constant = getType<VmConstant>(address))
	{
		if(constant->container)
			return constant;
	}

	return NULL;
}

VmConstant* GetLoadRegisterAndOffset(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmValue *address, VmValue *offset, unsigned char &addressReg)
{
	VmConstant *constantOffset = getType<VmConstant>(offset);

	if(VmConstant *constant = GetAddressInContainer(address))
	{
		assert(constantOffset->iValue == 0);

		addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

		return constant;
	}

	addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, address);

	return constantOffset;
}

void LowerBinaryOperationIntoBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmInstruction *inst, unsigned char lhsReg, RegVmInstructionCode iCode, RegVmInstructionCode dCode, RegVmInstructionCode lCode)
{
	VmValueType lhsType = inst->arguments[0]->type.type;

	if(unsigned constantIndex = TryLowerConstantToMemory(lowBlock, inst->arguments[1]))
	{
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(lhsType == VM_TYPE_INT || (lhsType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4))
			lowBlock->AddInstruction(ctx, inst->source, iCode, targetReg, lhsReg, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
		else if(lhsType == VM_TYPE_DOUBLE && dCode != rviNop)
			lowBlock->AddInstruction(ctx, inst->source, dCode, targetReg, lhsReg, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
		else if(lhsType == VM_TYPE_LONG || (lhsType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8))
			lowBlock->AddInstruction(ctx, inst->source, lCode, targetReg, lhsReg, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
		else
			assert(!"unknown type");
	}
	else
	{
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(lhsType == VM_TYPE_INT || (lhsType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4))
			lowBlock->AddInstruction(ctx, inst->source, iCode, targetReg, lhsReg, rvrrRegisters, rhsReg * sizeof(RegVmRegister));
		else if(lhsType == VM_TYPE_DOUBLE && dCode != rviNop)
			lowBlock->AddInstruction(ctx, inst->source, dCode, targetReg, lhsReg, rvrrRegisters, rhsReg * sizeof(RegVmRegister));
		else if(lhsType == VM_TYPE_LONG || (lhsType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8))
			lowBlock->AddInstruction(ctx, inst->source, lCode, targetReg, lhsReg, rvrrRegisters, rhsReg * sizeof(RegVmRegister));
		else
			assert(!"unknown type");
	}
}

void LowerBinaryMemoryOperationIntoBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmInstruction *inst, RegVmInstructionCode iCode, RegVmInstructionCode fCode, RegVmInstructionCode dCode, RegVmInstructionCode lCode)
{
	unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
	unsigned char addressReg = 0;
	VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[1], inst->arguments[2], addressReg);
	VmConstant *loadType = getType<VmConstant>(inst->arguments[3]);
	unsigned char targetReg = lowFunction->AllocateRegister(inst);

	VmType type = inst->arguments[0]->type;

	if(type == VmType::Int || (type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4))
		lowBlock->AddInstruction(ctx, inst->source, iCode, targetReg, lhsReg, addressReg, constant);
	else if(type == VmType::Double && loadType->iValue == VM_INST_LOAD_FLOAT && fCode != rviNop)
		lowBlock->AddInstruction(ctx, inst->source, fCode, targetReg, lhsReg, addressReg, constant);
	else if(type == VmType::Double && loadType->iValue == VM_INST_LOAD_DOUBLE && dCode != rviNop)
		lowBlock->AddInstruction(ctx, inst->source, dCode, targetReg, lhsReg, addressReg, constant);
	else if(type == VmType::Long || (type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8))
		lowBlock->AddInstruction(ctx, inst->source, lCode, targetReg, lhsReg, addressReg, constant);
	else
		assert(!"unknown type");
}

void LowerInstructionIntoBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmValue *value)
{
	RegVmLoweredInstruction *lastLowered = lowBlock->lastInstruction;

	VmInstruction *inst = getType<VmInstruction>(value);

	assert(inst);

	switch(inst->cmd)
	{
	case VM_INST_LOAD_BYTE:
	{
		unsigned char addressReg = 0;
		VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], addressReg);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLoadByte, targetReg, 0, addressReg, constant);
	}
		break;
	case VM_INST_LOAD_SHORT:
	{
		unsigned char addressReg = 0;
		VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], addressReg);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLoadWord, targetReg, 0, addressReg, constant);
	}
		break;
	case VM_INST_LOAD_INT:
	{
		unsigned char addressReg = 0;
		VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], addressReg);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, targetReg, 0, addressReg, constant);
	}
		break;
	case VM_INST_LOAD_FLOAT:
	{
		unsigned char addressReg = 0;
		VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], addressReg);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLoadFloat, targetReg, 0, addressReg, constant);
	}
		break;
	case VM_INST_LOAD_DOUBLE:
	{
		unsigned char addressReg = 0;
		VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], addressReg);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLoadDouble, targetReg, 0, addressReg, constant);
	}
		break;
	case VM_INST_LOAD_LONG:
	{
		unsigned char addressReg = 0;
		VmConstant *constant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], addressReg);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, targetReg, 0, addressReg, constant);
	}
		break;
	case VM_INST_LOAD_STRUCT:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			assert((unsigned short)inst->type.size == inst->type.size);

			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			unsigned pos = 0;

			if(inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
			}
			else if(inst->type.type == VM_TYPE_AUTO_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 8;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
			}
			else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, 1, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 2, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, false), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 2, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
			}
			else if(inst->type.type == VM_TYPE_STRUCT)
			{
				unsigned index = 0;

				unsigned remainingSize = inst->type.size;

				assert(remainingSize % 4 == 0);

				while(remainingSize != 0)
				{
					if(remainingSize == 4)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, index++, true), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
						pos += 4;

						remainingSize -= 4;
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, index++, remainingSize == 8), 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
						pos += 8;

						remainingSize -= 8;
					}
				}
			}
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			assert((unsigned short)inst->type.size == inst->type.size);

			unsigned pos = offset->iValue;

			if(inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->type.type == VM_TYPE_AUTO_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, pos);
					pos += 8;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, true), 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, 1, false), 0, addressReg, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 2, true), 0, addressReg, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 0, false), 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 1, false), 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, 2, true), 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->type.type == VM_TYPE_STRUCT)
			{
				unsigned index = 0;

				unsigned remainingSize = inst->type.size;

				assert(remainingSize % 4 == 0);

				while(remainingSize != 0)
				{
					if(remainingSize == 4)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, lowFunction->AllocateRegister(inst, index++, true), 0, addressReg, pos);
						pos += 4;

						remainingSize -= 4;
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviLoadLong, lowFunction->AllocateRegister(inst, index++, remainingSize == 8), 0, addressReg, pos);
						pos += 8;

						remainingSize -= 8;
					}
				}
			}
		}
		break;
	case VM_INST_LOAD_IMMEDIATE:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			// Handle all integers but other types only with a value of 0
			if(constant->type == VmType::Int)
			{
				lowBlock->AddInstruction(ctx, inst->source, rviLoadImm, targetReg, 0, 0, constant);
			}
			else if(constant->type == VmType::Double)
			{
				unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

				assert(constantIndex);

				lowBlock->AddInstruction(ctx, constant->source, rviLoadDouble, targetReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
			}
			else if(constant->type == VmType::Long)
			{
				unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

				assert(constantIndex);

				lowBlock->AddInstruction(ctx, constant->source, rviLoadLong, targetReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
			}
			else if(constant->type.type == VM_TYPE_POINTER)
			{
				if(constant->container)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviGetAddr, targetReg, 0, IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals, constant);
				}
				else
				{
					assert(!constant->container);

					if(NULLC_PTR_SIZE == 8)
					{
						unsigned constantIndex = TryLowerConstantToMemory(lowBlock, constant);

						assert(constantIndex);

						lowBlock->AddInstruction(ctx, constant->source, rviLoadLong, targetReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviLoadImm, targetReg, 0, 0, 0u);
					}
				}
			}
			else
			{
				assert(!"unknown type");
			}
		}
		break;
	case VM_INST_STORE_BYTE:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			lowBlock->AddInstruction(ctx, inst->source, rviStoreByte, sourceReg, 0, addressReg, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreByte, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_SHORT:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			lowBlock->AddInstruction(ctx, inst->source, rviStoreWord, sourceReg, 0, addressReg, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreWord, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_INT:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceReg, 0, addressReg, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_FLOAT:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			lowBlock->AddInstruction(ctx, inst->source, rviStoreFloat, sourceReg, 0, addressReg, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreFloat, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_LONG:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceReg, 0, addressReg, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_DOUBLE:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			lowBlock->AddInstruction(ctx, inst->source, rviStoreDouble, sourceReg, 0, addressReg, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreDouble, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_STRUCT:
		if(VmConstant *constant = GetAddressInContainer(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			VmConstant *constantRhs = getType<VmConstant>(inst->arguments[2]);

			assert((unsigned short)inst->arguments[2]->type.size == inst->arguments[2]->type.size);

			if(constantRhs && inst->arguments[2]->type.type == VM_TYPE_STRUCT)
			{
				if(unsigned constantIndex = TryLowerConstantToMemory(lowBlock, inst->arguments[2]))
				{
					unsigned char targetReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

					unsigned char sourceReg = lowFunction->GetRegisterForConstant();
					lowBlock->AddInstruction(ctx, inst->source, rviGetAddr, sourceReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));

					lowBlock->AddInstruction(ctx, inst->source, rviMemCopy, targetReg, 0, sourceReg, inst->arguments[2]->type.size);
					break;
				}
			}

			SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
			GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[2]);

			unsigned char addressReg = IsLocalScope(constant->container->scope) ? rvrrFrame : rvrrGlobals;

			unsigned pos = 0;

			if(inst->arguments[2]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[2]->type.type == VM_TYPE_ARRAY_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[0], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[1], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 8;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_ARRAY)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[1], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[2], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[2], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_STRUCT)
			{
				unsigned remainingSize = inst->arguments[2]->type.size;

				assert(remainingSize % 4 == 0);

				for(unsigned i = 0; i < sourceRegs.size(); i++)
				{
					if(remainingSize == 4)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[i], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
						pos += 4;

						remainingSize -= 4;
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[i], 0, addressReg, CreateConstantPointer(ctx.allocator, constant->source, constant->iValue + pos, constant->container, constant->type.structType, false));
						pos += 8;

						remainingSize -= 8;
					}
				}
			}
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			VmConstant *constantRhs = getType<VmConstant>(inst->arguments[2]);

			assert((unsigned short)inst->arguments[2]->type.size == inst->arguments[2]->type.size);

			if(constantRhs && inst->arguments[2]->type.type == VM_TYPE_STRUCT)
			{
				if(unsigned constantIndex = TryLowerConstantToMemory(lowBlock, inst->arguments[2]))
				{
					unsigned char targetReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

					if(offset->iValue != 0)
					{
						unsigned char dstAddressReg = lowFunction->GetRegisterForConstant();

						if(NULLC_PTR_SIZE == 4)
							lowBlock->AddInstruction(ctx, inst->source, rviAddImm, dstAddressReg, targetReg, 0, offset->iValue);
						else
							lowBlock->AddInstruction(ctx, inst->source, rviAddImml, dstAddressReg, targetReg, 0, offset->iValue);

						targetReg = dstAddressReg;
					}

					unsigned char sourceReg = lowFunction->GetRegisterForConstant();
					lowBlock->AddInstruction(ctx, inst->source, rviGetAddr, sourceReg, 0, rvrrConstants, (constantIndex - 1) * sizeof(unsigned));

					lowBlock->AddInstruction(ctx, inst->source, rviMemCopy, targetReg, 0, sourceReg, inst->arguments[2]->type.size);
					break;
				}
			}

			SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
			GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[2]);

			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			unsigned pos = offset->iValue;

			if(inst->arguments[2]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[2]->type.type == VM_TYPE_ARRAY_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[0], 0, addressReg, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[1], 0, addressReg, pos);
					pos += 8;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_ARRAY)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[1], 0, addressReg, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[2], 0, addressReg, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[1], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[2], 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_STRUCT)
			{
				unsigned remainingSize = inst->arguments[2]->type.size;

				assert(remainingSize % 4 == 0);

				for(unsigned i = 0; i < sourceRegs.size(); i++)
				{
					if(remainingSize == 4)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceRegs[i], 0, addressReg, pos);
						pos += 4;

						remainingSize -= 4;
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreLong, sourceRegs[i], 0, addressReg, pos);
						pos += 8;

						remainingSize -= 8;
					}
				}
			}
		}
		break;
	case VM_INST_DOUBLE_TO_INT:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviDtoi, targetReg, 0, sourceReg);
	}
		break;
	case VM_INST_DOUBLE_TO_LONG:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviDtol, targetReg, 0, sourceReg);
	}
		break;
	case VM_INST_DOUBLE_TO_FLOAT:
		if(VmConstant *argument = getType<VmConstant>(inst->arguments[0]))
		{
			float result = float(argument->dValue);

			unsigned target = 0;
			memcpy(&target, &result, sizeof(float));

			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadImm, targetReg, 0, 0, target);
		}
		else
		{
			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviDtof, targetReg, 0, sourceReg);
		}
		break;
	case VM_INST_INT_TO_DOUBLE:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviItod, targetReg, 0, sourceReg);
	}
		break;
	case VM_INST_LONG_TO_DOUBLE:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLtod, targetReg, 0, sourceReg);
	}
		break;
	case VM_INST_INT_TO_LONG:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviItol, targetReg, 0, sourceReg);
	}
		break;
	case VM_INST_LONG_TO_INT:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviLtoi, targetReg, 0, sourceReg);
	}
		break;
	case VM_INST_INDEX:
	{
		VmConstant *arrSize = getType<VmConstant>(inst->arguments[0]);
		VmConstant *elementSize = getType<VmConstant>(inst->arguments[1]);
		VmValue *pointer = inst->arguments[2];
		VmValue *index = inst->arguments[3];

		assert(arrSize && elementSize);
		assert((unsigned short)elementSize->iValue == elementSize->iValue);

		if(VmConstant *constantIndex = getType<VmConstant>(index))
		{
			if(unsigned(constantIndex->iValue) < unsigned(arrSize->iValue))
			{
				if(constantIndex->iValue == 0)
				{
					unsigned char pointerReg = GetArgumentRegister(ctx, lowFunction, lowBlock, pointer);

					if(!lowFunction->TransferRegisterTo(inst, pointerReg))
					{
						unsigned char targetReg = lowFunction->AllocateRegister(inst, 0, false);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, targetReg, 0, pointerReg);
					}
				}
				else
				{
					unsigned char pointerReg = GetArgumentRegister(ctx, lowFunction, lowBlock, pointer);
					unsigned char targetReg = lowFunction->AllocateRegister(inst);

					if(NULLC_PTR_SIZE == 4)
						lowBlock->AddInstruction(ctx, inst->source, rviAddImm, targetReg, pointerReg, 0, constantIndex->iValue * elementSize->iValue);
					else
						lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, pointerReg, 0, constantIndex->iValue * elementSize->iValue);
				}

				break;
			}
		}

		unsigned char indexReg = GetArgumentRegister(ctx, lowFunction, lowBlock, index);
		unsigned char pointerReg = GetArgumentRegister(ctx, lowFunction, lowBlock, pointer);
		unsigned char arrSizeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, arrSize);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviIndex, targetReg, indexReg, pointerReg, arrSizeReg << 16 | (unsigned short)elementSize->iValue);
	}
	break;
	case VM_INST_INDEX_UNSIZED:
	{
		VmConstant *elementSize = getType<VmConstant>(inst->arguments[0]);
		VmValue *arr = inst->arguments[1];
		VmValue *index = inst->arguments[2];

		assert(elementSize);

		unsigned char indexReg = GetArgumentRegister(ctx, lowFunction, lowBlock, index);

		SmallArray<unsigned char, 32> arrRegs(ctx.allocator);
		GetArgumentRegisters(ctx, lowFunction, lowBlock, arrRegs, arr);

		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		assert((unsigned short)elementSize->iValue == elementSize->iValue);

		lowBlock->AddInstruction(ctx, inst->source, rviIndex, targetReg, indexReg, arrRegs[0], arrRegs[1] << 16 | (unsigned short)elementSize->iValue);
	}
	break;
	case VM_INST_FUNCTION_ADDRESS:
	{
		VmConstant *funcIndex = getType<VmConstant>(inst->arguments[0]);

		assert(funcIndex);

		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviFuncAddr, targetReg, 0, 0, funcIndex);
	}
	break;
	case VM_INST_TYPE_ID:
	{
		VmConstant *typeIndex = getType<VmConstant>(inst->arguments[0]);

		assert(typeIndex);

		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviTypeid, targetReg, 0, 0, typeIndex->iValue);
	}
	break;
	case VM_INST_SET_RANGE:
	{
		VmValue *address = inst->arguments[0];
		VmConstant *count = getType<VmConstant>(inst->arguments[1]);
		VmValue *initializer = inst->arguments[2];
		VmConstant *elementSize = getType<VmConstant>(inst->arguments[3]);

		unsigned char initializerReg = GetArgumentRegister(ctx, lowFunction, lowBlock, initializer);
		unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, address);

		if(initializer->type == VmType::Int && elementSize->iValue == 1)
			lowBlock->AddInstruction(ctx, inst->source, rviSetRange, initializerReg, rvsrChar, addressReg, count->iValue);
		else if(initializer->type == VmType::Int && elementSize->iValue == 2)
			lowBlock->AddInstruction(ctx, inst->source, rviSetRange, initializerReg, rvsrShort, addressReg, count->iValue);
		else if(initializer->type == VmType::Int && elementSize->iValue == 4)
			lowBlock->AddInstruction(ctx, inst->source, rviSetRange, initializerReg, rvsrInt, addressReg, count->iValue);
		else if(initializer->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviSetRange, initializerReg, rvsrLong, addressReg, count->iValue);
		else if(initializer->type == VmType::Double && elementSize->iValue == 4)
			lowBlock->AddInstruction(ctx, inst->source, rviSetRange, initializerReg, rvsrFloat, addressReg, count->iValue);
		else if(initializer->type == VmType::Double && elementSize->iValue == 8)
			lowBlock->AddInstruction(ctx, inst->source, rviSetRange, initializerReg, rvsrDouble, addressReg, count->iValue);
	}
		break;
	case VM_INST_MEM_COPY:
	{
		unsigned char dstAddressReg = 0;
		VmConstant *dstConstant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[0], inst->arguments[1], dstAddressReg);

		if(dstConstant->container)
		{
			dstAddressReg = lowFunction->GetRegisterForConstant();

			lowBlock->AddInstruction(ctx, inst->source, rviGetAddr, dstAddressReg, 0, IsLocalScope(dstConstant->container->scope) ? rvrrFrame : rvrrGlobals, dstConstant);
		}
		else if(dstConstant->iValue != 0)
		{
			unsigned char targetReg = lowFunction->GetRegisterForConstant();

			if (NULLC_PTR_SIZE == 4)
				lowBlock->AddInstruction(ctx, inst->source, rviAddImm, targetReg, dstAddressReg, 0, dstConstant->iValue);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, dstAddressReg, 0, dstConstant->iValue);

			dstAddressReg = targetReg;
		}

		unsigned char srcAddressReg = 0;
		VmConstant *srcConstant = GetLoadRegisterAndOffset(ctx, lowFunction, lowBlock, inst->arguments[2], inst->arguments[3], srcAddressReg);

		if(srcConstant->container)
		{
			srcAddressReg = lowFunction->GetRegisterForConstant();

			lowBlock->AddInstruction(ctx, inst->source, rviGetAddr, srcAddressReg, 0, IsLocalScope(srcConstant->container->scope) ? rvrrFrame : rvrrGlobals, srcConstant);
		}
		else if(srcConstant->iValue != 0)
		{
			unsigned char targetReg = lowFunction->GetRegisterForConstant();

			if(NULLC_PTR_SIZE == 4)
				lowBlock->AddInstruction(ctx, inst->source, rviAddImm, targetReg, srcAddressReg, 0, srcConstant->iValue);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, srcAddressReg, 0, srcConstant->iValue);

			srcAddressReg = targetReg;
		}

		VmConstant *size = getType<VmConstant>(inst->arguments[4]);

		lowBlock->AddInstruction(ctx, inst->source, rviMemCopy, dstAddressReg, 0, srcAddressReg, size->iValue);
	}
	break;
	case VM_INST_JUMP:
		// Check if jump is fall-through (except for fallthrough from empty entry block)
		if(!(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[0]) || (lowBlock->vmBlock->prevSibling == NULL && lowBlock->firstInstruction == NULL))
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmp, 0, 0, 0, getType<VmBlock>(inst->arguments[0]));
		}
		break;
	case VM_INST_JUMP_Z:
	{
		assert(inst->arguments[0]->type.size == 4);

		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		// Check if one side of the jump is fall-through
		if(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[1])
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmpnz, 0, 0, sourceReg, getType<VmBlock>(inst->arguments[2]));
		}
		else
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmpz, 0, 0, sourceReg, getType<VmBlock>(inst->arguments[1]));

			lowBlock->AddInstruction(ctx, inst->source, rviJmp, 0, 0, 0, getType<VmBlock>(inst->arguments[2]));
		}
	}
		break;
	case VM_INST_JUMP_NZ:
	{
		assert(inst->arguments[0]->type.size == 4);

		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		// Check if one side of the jump is fall-through
		if(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[1])
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmpz, 0, 0, sourceReg, getType<VmBlock>(inst->arguments[2]));
		}
		else
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmpnz, 0, 0, sourceReg, getType<VmBlock>(inst->arguments[1]));

			lowBlock->AddInstruction(ctx, inst->source, rviJmp, 0, 0, 0, getType<VmBlock>(inst->arguments[2]));
		}
	}
		break;
	case VM_INST_CALL:
	{
		assert((unsigned short)inst->type.size == inst->type.size);

		RegVmInstructionCode targetInst = rviNop;

		VmValue *targetContext = NULL;
		VmFunction *targetFunction = NULL;

		VmValue *resultTarget = NULL;

		unsigned firstArgument = ~0u;

		SmallArray<unsigned char, 32> targetRegs(ctx.allocator);

		if(inst->arguments[0]->type.type == VM_TYPE_FUNCTION_REF)
		{
			GetArgumentRegisters(ctx, lowFunction, lowBlock, targetRegs, inst->arguments[0]);

			resultTarget = inst->arguments[1];

			targetInst = rviCallPtr;

			firstArgument = 2;
		}
		else
		{
			targetContext = inst->arguments[0];
			targetFunction = getType<VmFunction>(inst->arguments[1]);

			resultTarget = inst->arguments[2];

			if(VmConstant *context = getType<VmConstant>(targetContext))
			{
				if(context->container)
				{
					LowerConstantIntoBlock(ctx, lowFunction, lowBlock, targetRegs, context);
					targetRegs.push_back(0);
				}
				else
				{
					targetRegs.push_back(0);
					targetRegs.push_back(0);
				}
			}
			else
			{
				GetArgumentRegisters(ctx, lowFunction, lowBlock, targetRegs, targetContext);
				targetRegs.push_back(0);
			}

			targetInst = rviCall;

			firstArgument = 3;
		}

		VmConstant *resultAddress = getType<VmConstant>(resultTarget);

		RegVmLoweredModule *lowModule = lowBlock->parent->parent;

		unsigned microcodePos = lowModule->constants.size();

		for(unsigned i = firstArgument; i < inst->arguments.size(); i++)
		{
			VmValue *argument = inst->arguments[i];

			if(argument->type.size == 0)
			{
				lowModule->constants.push_back(rvmiPushImm);
				lowModule->constants.push_back(0);
			}
			else
			{
				if(TryLowerConstantPushIntoBlock(lowBlock, argument))
					continue;

				unsigned constantCount = lowModule->constants.size();

				SmallArray<unsigned char, 32> argumentRegs(ctx.allocator);
				GetArgumentRegisters(ctx, lowFunction, lowBlock, argumentRegs, argument);

				assert(constantCount == lowModule->constants.size() && "can't add constants while call microcode is prepared");
				(void)constantCount;

				if(argument->type.type == VM_TYPE_INT || (NULLC_PTR_SIZE == 4 && argument->type.type == VM_TYPE_POINTER))
				{
					lowModule->constants.push_back(rvmiPush);
					lowModule->constants.push_back(argumentRegs[0]);
				}
				else if(argument->type.type == VM_TYPE_LONG || (NULLC_PTR_SIZE == 8 && argument->type.type == VM_TYPE_POINTER))
				{
					lowModule->constants.push_back(rvmiPushQword);
					lowModule->constants.push_back(argumentRegs[0]);
				}
				else if(argument->type.type == VM_TYPE_DOUBLE)
				{
					lowModule->constants.push_back(rvmiPushQword);
					lowModule->constants.push_back(argumentRegs[0]);
				}
				else if(argument->type.type == VM_TYPE_FUNCTION_REF || argument->type.type == VM_TYPE_ARRAY_REF)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowModule->constants.push_back(rvmiPushQword);
						lowModule->constants.push_back(argumentRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[1]);
					}
					else
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[1]);
					}
				}
				else if(argument->type.type == VM_TYPE_AUTO_REF)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[0]);

						lowModule->constants.push_back(rvmiPushQword);
						lowModule->constants.push_back(argumentRegs[1]);
					}
					else
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[1]);
					}
				}
				else if(argument->type.type == VM_TYPE_AUTO_ARRAY)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[0]);

						lowModule->constants.push_back(rvmiPushQword);
						lowModule->constants.push_back(argumentRegs[1]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[2]);
					}
					else
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[1]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(argumentRegs[2]);
					}
				}
				else if(argument->type.type == VM_TYPE_STRUCT)
				{
					unsigned remainingSize = argument->type.size;

					for(unsigned i = 0; i < argumentRegs.size(); i++)
					{
						if(remainingSize == 4)
						{
							lowModule->constants.push_back(rvmiPush);
							lowModule->constants.push_back(argumentRegs[i]);

							remainingSize -= 4;
						}
						else
						{
							lowModule->constants.push_back(rvmiPushQword);
							lowModule->constants.push_back(argumentRegs[i]);

							remainingSize -= 8;
						}
					}

					assert(remainingSize == 0);
				}
			}
		}

		if(VmConstant *context = getType<VmConstant>(targetContext))
		{
			if(context->container)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowModule->constants.push_back(rvmiPushQword);
					lowModule->constants.push_back(targetRegs[0]);
				}
				else
				{
					lowModule->constants.push_back(rvmiPush);
					lowModule->constants.push_back(targetRegs[0]);
				}
			}
			else
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowModule->constants.push_back(rvmiPushImmq);
					lowModule->constants.push_back(context->iValue);
				}
				else
				{
					lowModule->constants.push_back(rvmiPushImm);
					lowModule->constants.push_back(context->iValue);
				}
			}
		}
		else
		{
			if(NULLC_PTR_SIZE == 8)
			{
				lowModule->constants.push_back(rvmiPushQword);
				lowModule->constants.push_back(targetRegs[0]);
			}
			else
			{
				lowModule->constants.push_back(rvmiPush);
				lowModule->constants.push_back(targetRegs[0]);
			}
		}

		bool fakeUser = false;

		if(inst->type.type != VM_TYPE_VOID && inst->users.empty())
		{
			fakeUser = true;
			inst->users.push_back(NULL);
		}

		unsigned char regA = (microcodePos >> 16) & 0xff, regB = (microcodePos >> 8) & 0xff, regC = microcodePos & 0xff;

		if(inst->type.type == VM_TYPE_VOID)
		{
			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(rvrVoid);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);
		}
		else if(inst->type.type == VM_TYPE_INT || (NULLC_PTR_SIZE == 4 && inst->type.type == VM_TYPE_POINTER))
		{
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(targetReg);
			lowModule->constants.push_back(rvrInt);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);
		}
		else if(inst->type.type == VM_TYPE_DOUBLE || inst->type.type == VM_TYPE_LONG || (NULLC_PTR_SIZE == 8 && inst->type.type == VM_TYPE_POINTER))
		{
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(targetReg);
			lowModule->constants.push_back(inst->type.type == VM_TYPE_DOUBLE ? rvrDouble : rvrLong);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);
		}
		else if(inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF)
		{
			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(rvrStruct);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);

			unsigned char regA = lowFunction->AllocateRegister(inst, 0, false);
			unsigned char regB = lowFunction->AllocateRegister(inst, 1, true);

			if(NULLC_PTR_SIZE == 8)
			{
				lowModule->constants.push_back(rvmiPopq);
				lowModule->constants.push_back(regA);

				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regB);
			}
			else
			{
				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regA);

				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regB);
			}
		}
		else if(inst->type.type == VM_TYPE_AUTO_REF)
		{
			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(rvrStruct);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);

			unsigned char regA = lowFunction->AllocateRegister(inst, 0, false);
			unsigned char regB = lowFunction->AllocateRegister(inst, 1, true);

			if(NULLC_PTR_SIZE == 8)
			{
				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regA);

				lowModule->constants.push_back(rvmiPopq);
				lowModule->constants.push_back(regB);
			}
			else
			{
				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regA);

				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regB);
			}
		}
		else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
		{
			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(rvrStruct);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);

			unsigned char regA = lowFunction->AllocateRegister(inst, 0, false);
			unsigned char regB = lowFunction->AllocateRegister(inst, 1, false);
			unsigned char regC = lowFunction->AllocateRegister(inst, 2, true);

			if(NULLC_PTR_SIZE == 8)
			{
				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regA);

				lowModule->constants.push_back(rvmiPopq);
				lowModule->constants.push_back(regB);

				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regC);
			}
			else
			{
				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regA);

				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regB);

				lowModule->constants.push_back(rvmiPop);
				lowModule->constants.push_back(regC);
			}
		}
		else if(inst->type.type == VM_TYPE_STRUCT && resultAddress && resultAddress->isReference)
		{
			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(rvrStruct);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);

			lowModule->constants.push_back(rvmiPopMem);
			lowModule->constants.push_back(IsLocalScope(resultAddress->container->scope) ? rvrrFrame : rvrrGlobals);
			lowModule->constants.push_back(resultAddress->container->offset);
			lowModule->constants.push_back(int(resultAddress->container->type->size));

		}
		else if(inst->type.type == VM_TYPE_STRUCT)
		{
			lowModule->constants.push_back(rvmiCall);
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(rvrStruct);

			if(targetInst == rviCall)
				lowBlock->AddInstruction(ctx, inst->source, rviCall, regA, regB, regC, targetFunction);
			else
				lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1], microcodePos);

			unsigned remainingSize = inst->type.size;

			for(unsigned i = 0; i < (remainingSize + 4) / 8; i++)
				lowFunction->AllocateRegister(inst, i, i + 1 == (remainingSize + 4) / 8);

			unsigned index = 0;

			while(remainingSize != 0)
			{
				if(remainingSize == 4)
				{
					lowModule->constants.push_back(rvmiPop);
					lowModule->constants.push_back(inst->regVmRegisters[index]);

					remainingSize -= 4;
				}
				else
				{
					lowModule->constants.push_back(rvmiPopq);
					lowModule->constants.push_back(inst->regVmRegisters[index]);

					remainingSize -= 8;
				}

				index++;
			}
		}

		lowModule->constants.push_back(rvmiReturn);

		if(fakeUser)
		{
			for(unsigned i = 0; i < inst->regVmRegisters.size(); i++)
				lowFunction->FreeRegister(inst->regVmRegisters[i]);

			inst->users.clear();
		}
	}
	break;
	case VM_INST_RETURN:
	case VM_INST_YIELD:
	{
		RegVmLoweredModule *lowModule = lowBlock->parent->parent;

		unsigned microcodePos = lowModule->constants.size();

		if(!inst->arguments.empty())
		{
			VmValue *result = inst->arguments[0];

			TypeBase *resultBaseType = GetBaseType(ctx, result->type);

			lowModule->constants.push_back(resultBaseType->typeIndex);
			lowModule->constants.push_back(unsigned(resultBaseType->size));

			RegVmReturnType resultType = rvrVoid;

			if(result->type.type == VM_TYPE_INT || (NULLC_PTR_SIZE == 4 && result->type.type == VM_TYPE_POINTER))
				resultType = rvrInt;
			else if(result->type.type == VM_TYPE_LONG || (NULLC_PTR_SIZE == 8 && result->type.type == VM_TYPE_POINTER))
				resultType = rvrLong;
			else if(result->type.type == VM_TYPE_DOUBLE)
				resultType = rvrDouble;
			else if(result->type.type == VM_TYPE_FUNCTION_REF || result->type.type == VM_TYPE_ARRAY_REF)
				resultType = rvrStruct;
			else if(result->type.type == VM_TYPE_AUTO_REF)
				resultType = rvrStruct;
			else if(result->type.type == VM_TYPE_AUTO_ARRAY)
				resultType = rvrStruct;
			else if(result->type.type == VM_TYPE_STRUCT)
				resultType = rvrStruct;

			if(!TryLowerConstantPushIntoBlock(lowBlock, result))
			{
				unsigned constantCount = lowModule->constants.size();

				SmallArray<unsigned char, 32> resultRegs(ctx.allocator);
				GetArgumentRegisters(ctx, lowFunction, lowBlock, resultRegs, result);

				assert(constantCount == lowModule->constants.size() && "can't add constants while return microcode is prepared");
				(void)constantCount;

				if(result->type.type == VM_TYPE_INT || (NULLC_PTR_SIZE == 4 && result->type.type == VM_TYPE_POINTER))
				{
					lowModule->constants.push_back(rvmiPush);
					lowModule->constants.push_back(resultRegs[0]);
				}
				else if(result->type.type == VM_TYPE_LONG || (NULLC_PTR_SIZE == 8 && result->type.type == VM_TYPE_POINTER))
				{
					lowModule->constants.push_back(rvmiPushQword);
					lowModule->constants.push_back(resultRegs[0]);
				}
				else if(result->type.type == VM_TYPE_DOUBLE)
				{
					lowModule->constants.push_back(rvmiPushQword);
					lowModule->constants.push_back(resultRegs[0]);
				}
				else if(result->type.type == VM_TYPE_FUNCTION_REF || result->type.type == VM_TYPE_ARRAY_REF)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowModule->constants.push_back(rvmiPushQword);
						lowModule->constants.push_back(resultRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[1]);
					}
					else
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[1]);
					}
				}
				else if(result->type.type == VM_TYPE_AUTO_REF)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[0]);

						lowModule->constants.push_back(rvmiPushQword);
						lowModule->constants.push_back(resultRegs[1]);
					}
					else
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[1]);
					}
				}
				else if(result->type.type == VM_TYPE_AUTO_ARRAY)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[0]);

						lowModule->constants.push_back(rvmiPushQword);
						lowModule->constants.push_back(resultRegs[1]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[2]);
					}
					else
					{
						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[0]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[1]);

						lowModule->constants.push_back(rvmiPush);
						lowModule->constants.push_back(resultRegs[2]);
					}
				}
				else if(result->type.type == VM_TYPE_STRUCT)
				{
					unsigned remainingSize = result->type.size;

					for(unsigned i = 0; i < resultRegs.size(); i++)
					{
						if(remainingSize == 4)
						{
							lowModule->constants.push_back(rvmiPush);
							lowModule->constants.push_back(resultRegs[i]);

							remainingSize -= 4;
						}
						else
						{
							lowModule->constants.push_back(rvmiPushQword);
							lowModule->constants.push_back(resultRegs[i]);

							remainingSize -= 8;
						}
					}
				}
			}

			unsigned char checkReturn = isType<TypeRef>(resultBaseType) || isType<TypeUnsizedArray>(resultBaseType);

			lowModule->constants.push_back(rvmiReturn);

			lowBlock->AddInstruction(ctx, inst->source, rviReturn, 0, (unsigned char)resultType, checkReturn, microcodePos);
		}
		else
		{
			lowModule->constants.push_back(0);
			lowModule->constants.push_back(0);

			lowModule->constants.push_back(rvmiReturn);

			lowBlock->AddInstruction(ctx, inst->source, rviReturn, 0, rvrVoid, 0, microcodePos);
		}
	}
	break;
	case VM_INST_ADD:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		if(VmConstant *constant = getType<VmConstant>(inst->arguments[1]))
		{
			if((inst->type == VmType::Int || (inst->type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4)) && constant->type == VmType::Int)
			{
				unsigned char targetReg = lowFunction->AllocateRegister(inst);

				lowBlock->AddInstruction(ctx, inst->source, rviAddImm, targetReg, lhsReg, 0, constant->iValue);
				break;
			}
			else if((inst->type == VmType::Long || (inst->type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8)) && (constant->type == VmType::Int || inst->type == VmType::Long) && int(constant->lValue) == constant->lValue)
			{
				unsigned char targetReg = lowFunction->AllocateRegister(inst);

				if(constant->type == VmType::Int)
					lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, lhsReg, 0, constant->iValue);
				else if(inst->type == VmType::Long)
					lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, lhsReg, 0, int(constant->lValue));
				else
					assert(!"unknown type");
				break;
			}
		}

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviAdd, rviAddd, rviAddl);
	}
	break;
	case VM_INST_SUB:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		if(VmConstant *constant = getType<VmConstant>(inst->arguments[1]))
		{
			if((inst->type == VmType::Int || (inst->type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4)) && constant->type == VmType::Int)
			{
				unsigned char targetReg = lowFunction->AllocateRegister(inst);

				lowBlock->AddInstruction(ctx, inst->source, rviAddImm, targetReg, lhsReg, 0, -constant->iValue);
				break;
			}
			else if((inst->type == VmType::Long || (inst->type.type == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8)) && (constant->type == VmType::Int || inst->type == VmType::Long) && int(constant->lValue) == constant->lValue)
			{
				unsigned char targetReg = lowFunction->AllocateRegister(inst);

				if(constant->type == VmType::Int)
					lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, lhsReg, 0, -constant->iValue);
				else if(inst->type == VmType::Long)
					lowBlock->AddInstruction(ctx, inst->source, rviAddImml, targetReg, lhsReg, 0, -int(constant->lValue));
				else
					assert(!"unknown type");
				break;
			}
		}

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviSub, rviSubd, rviSubl);
	}
	break;
	case VM_INST_MUL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviMul, rviMuld, rviMull);
	}
		break;
	case VM_INST_DIV:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviDiv, rviDivd, rviDivl);
	}
		break;
	case VM_INST_POW:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviPow, rviPowd, rviPowl);
	}
		break;
	case VM_INST_MOD:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviMod, rviModd, rviModl);
	}
		break;
	case VM_INST_LESS:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviLess, rviLessd, rviLessl);
	}
		break;
	case VM_INST_GREATER:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviGreater, rviGreaterd, rviGreaterl);
	}
		break;
	case VM_INST_LESS_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviLequal, rviLequald, rviLequall);
	}
		break;
	case VM_INST_GREATER_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviGequal, rviGequald, rviGequall);
	}
		break;
	case VM_INST_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviEqual, rviEquald, rviEquall);
	}
		break;
	case VM_INST_NOT_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviNequal, rviNequald, rviNequall);
	}
		break;
	case VM_INST_SHL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviShl, rviNop, rviShll);
	}
		break;
	case VM_INST_SHR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviShr, rviNop, rviShrl);
	}
		break;
	case VM_INST_BIT_AND:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviBitAnd, rviNop, rviBitAndl);
	}
		break;
	case VM_INST_BIT_OR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviBitOr, rviNop, rviBitOrl);
	}
		break;
	case VM_INST_BIT_XOR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

		LowerBinaryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, lhsReg, rviBitXor, rviNop, rviBitXorl);
	}
		break;
	case VM_INST_ADD_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviAdd, rviAddf, rviAddd, rviAddl);
	}
	break;
	case VM_INST_SUB_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviSub, rviSubf, rviSubd, rviSubl);
	}
	break;
	case VM_INST_MUL_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviMul, rviMulf, rviMuld, rviMull);
	}
	break;
	case VM_INST_DIV_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviDiv, rviDivf, rviDivd, rviDivl);
	}
	break;
	case VM_INST_POW_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviPow, rviNop, rviPowd, rviPowl);
	}
	break;
	case VM_INST_MOD_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviMod, rviNop, rviModd, rviModl);
	}
	break;
	case VM_INST_LESS_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviLess, rviNop, rviLessd, rviLessl);
	}
	break;
	case VM_INST_GREATER_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviGreater, rviNop, rviGreaterd, rviGreaterl);
	}
	break;
	case VM_INST_LESS_EQUAL_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviLequal, rviNop, rviLequald, rviLequall);
	}
	break;
	case VM_INST_GREATER_EQUAL_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviGequal, rviNop, rviGequald, rviGequall);
	}
	break;
	case VM_INST_EQUAL_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviEqual, rviNop, rviEquald, rviEquall);
	}
	break;
	case VM_INST_NOT_EQUAL_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviNequal, rviNop, rviNequald, rviNequall);
	}
	break;
	case VM_INST_SHL_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviShl, rviNop, rviNop, rviShll);
	}
	break;
	case VM_INST_SHR_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviShr, rviNop, rviNop, rviShrl);
	}
	break;
	case VM_INST_BIT_AND_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviBitAnd, rviNop, rviNop, rviBitAndl);
	}
	break;
	case VM_INST_BIT_OR_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviBitOr, rviNop, rviNop, rviBitOrl);
	}
	break;
	case VM_INST_BIT_XOR_LOAD:
	{
		LowerBinaryMemoryOperationIntoBlock(ctx, lowFunction, lowBlock, inst, rviBitXor, rviNop, rviNop, rviBitXorl);
	}
	break;
	case VM_INST_NEG:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviNeg, targetReg, 0, sourceReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviNegd, targetReg, 0, sourceReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviNegl, targetReg, 0, sourceReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_BIT_NOT:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviBitNot, targetReg, 0, sourceReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviBitNotl, targetReg, 0, sourceReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_LOG_NOT:
	{
		unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviLogNot, targetReg, 0, sourceReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviLogNotl, targetReg, 0, sourceReg);
		else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
			lowBlock->AddInstruction(ctx, inst->source, NULLC_PTR_SIZE == 4 ? rviLogNot : rviLogNotl, targetReg, 0, sourceReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_CONVERT_POINTER:
	{
		VmValue *pointer = inst->arguments[0];
		VmConstant *typeIndex = getType<VmConstant>(inst->arguments[1]);

		assert(pointer->type.type == VM_TYPE_AUTO_REF);
		assert(typeIndex);

		SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
		GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, pointer);

		bool fakeUser = false;

		if(inst->users.empty())
		{
			fakeUser = true;
			inst->users.push_back(NULL);
		}

		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		lowBlock->AddInstruction(ctx, inst->source, rviConvertPtr, targetReg, sourceRegs[0], sourceRegs[1], typeIndex->iValue);

		if(fakeUser)
		{
			for(unsigned i = 0; i < inst->regVmRegisters.size(); i++)
				lowFunction->FreeRegister(inst->regVmRegisters[i]);

			inst->users.clear();
		}
	}
	break;
	case VM_INST_ABORT_NO_RETURN:
	{
		RegVmLoweredModule *lowModule = lowBlock->parent->parent;

		unsigned microcodePos = lowModule->constants.size();

		lowModule->constants.push_back(0);
		lowModule->constants.push_back(0);

		lowModule->constants.push_back(rvmiReturn);

		lowBlock->AddInstruction(ctx, inst->source, rviReturn, 0, rvrError, 0, microcodePos);
	}
		break;
	case VM_INST_CONSTRUCT:
		if(inst->type.type == VM_TYPE_FUNCTION_REF)
		{
			unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char idReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, ptrReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, 0, false);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, idReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, idReg);
			}
		}
		else if(inst->type.type == VM_TYPE_ARRAY_REF)
		{
			unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char lenReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, ptrReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, 0, false);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, lenReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, lenReg);
			}
		}
		else if(inst->type.type == VM_TYPE_AUTO_REF)
		{
			unsigned char typeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, typeReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, 0, false);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, typeReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, ptrReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
			}
		}
		else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
		{
			if(inst->arguments.size() == 2)
			{
				unsigned char typeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

				SmallArray<unsigned char, 32> arrayRegs(ctx.allocator);
				GetArgumentRegisters(ctx, lowFunction, lowBlock, arrayRegs, inst->arguments[1]);

				if(!lowFunction->TransferRegisterTo(inst, typeReg))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, 0, false);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, typeReg);
				}

				if(!lowFunction->TransferRegisterTo(inst, arrayRegs[0]))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, false);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, arrayRegs[0]);
				}

				if(!lowFunction->TransferRegisterTo(inst, arrayRegs[1]))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, 2, true);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, arrayRegs[1]);
				}
			}
			else
			{
				unsigned char typeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
				unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
				unsigned char sizeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);

				if(!lowFunction->TransferRegisterTo(inst, typeReg))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, 0, false);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, typeReg);
				}

				if(!lowFunction->TransferRegisterTo(inst, ptrReg))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, false);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
				}

				if(!lowFunction->TransferRegisterTo(inst, sizeReg))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, 2, true);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sizeReg);
				}
			}
		}
		else
		{
			assert(!"unknown type");
		}
		break;
	case VM_INST_ARRAY:
	{
		assert(!"must be lagalized");

		assert(inst->arguments.size() <= 8);

		unsigned index = 0;

		for(unsigned i = 0; i < inst->arguments.size(); i++)
		{
			VmValue *argument = inst->arguments[i];

			assert(argument->type.size % 4 == 0);

			if(argument->type.size == 4)
			{
				if(i + 1 < inst->arguments.size())
				{
					unsigned char argReg1 = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[i]);
					unsigned char argReg2 = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[i + 1]);

					unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, true);

					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, copyReg, argReg1, argReg2);

					i++;
				}
				else
				{
					unsigned char argReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[i]);

					if(!lowFunction->TransferRegisterTo(inst, argReg))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, true);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, argReg);
					}
					else
					{
						index++;
					}
				}
			}
			else if(argument->type.size == 8)
			{
				unsigned char argReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[i]);

				if(!lowFunction->TransferRegisterTo(inst, argReg))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, true);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, argReg);
				}
				else
				{
					index++;
				}
			}
			else
			{
				SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
				GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[i]);

				for(unsigned k = 0; k < sourceRegs.size(); k++)
				{
					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[k]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, k + 1 == sourceRegs.size());

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[k]);
					}
					else
					{
						index++;
					}
				}
			}
		}
		break;
	}
	case VM_INST_EXTRACT:
		assert(!"invalid instruction");
		break;
	case VM_INST_UNYIELD:
	{
		unsigned char jumpPointReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char tempReg = lowFunction->GetRegisterForConstant();

		// Check secondary blocks first
		for(unsigned i = 2; i < inst->arguments.size(); i++)
		{
			lowBlock->AddInstruction(ctx, inst->source, rviLoadImm, tempReg, 0, 0, i - 1);
			lowBlock->AddInstruction(ctx, inst->source, rviEqual, tempReg, jumpPointReg, rvrrRegisters, tempReg * sizeof(RegVmRegister));
			lowBlock->AddInstruction(ctx, inst->source, rviJmpnz, 0, 0, tempReg, getType<VmBlock>(inst->arguments[i]));
		}

		// jump to entry block by default
		if(!(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[1]))
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmp, 0, 0, 0, getType<VmBlock>(inst->arguments[1]));
		}
	}
		break;
	case VM_INST_BITCAST:
	{
		if((inst->arguments[0]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[0]->type.type == VM_TYPE_ARRAY_REF || inst->arguments[0]->type.type == VM_TYPE_AUTO_REF || inst->arguments[0]->type.type == VM_TYPE_AUTO_ARRAY) && inst->type.type == VM_TYPE_STRUCT)
		{
			SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
			GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[0]);

			if(NULLC_PTR_SIZE == 8)
			{
				if(inst->arguments[0]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[0]->type.type == VM_TYPE_ARRAY_REF)
				{
					unsigned index = 0;

					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[0]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, false);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[0]);
					}
					else
					{
						index++;
					}

					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[1]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, index, true);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[1]);
					}
				}
				else if(inst->arguments[0]->type.type == VM_TYPE_AUTO_REF)
				{
					unsigned char resRegA = lowFunction->AllocateRegister(inst, 0, false);
					unsigned char resRegB = lowFunction->AllocateRegister(inst, 1, true);

					unsigned char tempRegA = lowFunction->GetRegisterForConstant();

					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, tempRegA, resRegB, sourceRegs[1], 32u);

					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, resRegA, sourceRegs[0], tempRegA);
				}
				else if(inst->arguments[0]->type.type == VM_TYPE_AUTO_ARRAY)
				{
					unsigned char resRegA = lowFunction->AllocateRegister(inst, 0, false);
					unsigned char resRegB = lowFunction->AllocateRegister(inst, 1, true);

					unsigned char tempRegA = lowFunction->GetRegisterForConstant();
					unsigned char tempRegB = lowFunction->GetRegisterForConstant();

					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, tempRegA, tempRegB, sourceRegs[1], 32u);

					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, resRegA, sourceRegs[0], tempRegA);
					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, resRegB, tempRegB, sourceRegs[2]);
				}
			}
			else
			{
				if(inst->arguments[0]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[0]->type.type == VM_TYPE_ARRAY_REF || inst->arguments[0]->type.type == VM_TYPE_AUTO_REF)
				{
					unsigned char combineReg = lowFunction->AllocateRegister(inst, 0, true);

					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, combineReg, sourceRegs[0], sourceRegs[1]);
				}
				else if(inst->arguments[0]->type.type == VM_TYPE_AUTO_ARRAY)
				{
					unsigned char combineReg = lowFunction->AllocateRegister(inst, 0, false);

					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, combineReg, sourceRegs[0], sourceRegs[1]);

					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[2]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, true);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[2]);
					}
				}
			}

			break;
		}

		if((inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF || inst->type.type == VM_TYPE_AUTO_REF || inst->type.type == VM_TYPE_AUTO_ARRAY) && inst->arguments[0]->type.type == VM_TYPE_STRUCT)
		{
			SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
			GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[0]);

			if(NULLC_PTR_SIZE == 8)
			{
				if(inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF)
				{
					unsigned index = 0;

					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[0]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, false);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[0]);
					}
					else
					{
						index++;
					}

					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[1]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, index, true);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[1]);
					}
				}
				else if(inst->type.type == VM_TYPE_AUTO_REF)
				{
					unsigned char resRegA = lowFunction->AllocateRegister(inst, 0, false);
					unsigned char resRegB = lowFunction->AllocateRegister(inst, 1, true);

					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, resRegA, resRegB, sourceRegs[0]);
					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, resRegB, resRegB, sourceRegs[1]);
				}
				else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
				{
					unsigned char resRegA = lowFunction->AllocateRegister(inst, 0, false);
					unsigned char resRegB = lowFunction->AllocateRegister(inst, 1, false);
					unsigned char resRegC = lowFunction->AllocateRegister(inst, 2, true);

					unsigned char tempReg = lowFunction->GetRegisterForConstant();

					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, resRegA, resRegB, sourceRegs[0]);
					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, tempReg, resRegC, sourceRegs[1]);
					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, resRegB, resRegB, tempReg);
				}
			}
			else
			{
				if(inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF || inst->type.type == VM_TYPE_AUTO_REF)
				{
					unsigned char resRegA = lowFunction->AllocateRegister(inst, 0, false);
					unsigned char resRegB = lowFunction->AllocateRegister(inst, 1, true);

					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, resRegA, resRegB, sourceRegs[0]);
				}
				else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
				{
					unsigned char resRegA = lowFunction->AllocateRegister(inst, 0, false);
					unsigned char resRegB = lowFunction->AllocateRegister(inst, 1, true);

					lowBlock->AddInstruction(ctx, inst->source, rviBreakupdd, resRegA, resRegB, sourceRegs[0]);

					if(!lowFunction->TransferRegisterTo(inst, sourceRegs[1]))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, 1, true);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[1]);
					}
				}
			}

			break;
		}

		if(inst->arguments[0]->type.type == VM_TYPE_STRUCT && !(inst->type.type == VM_TYPE_INT || inst->type.type == VM_TYPE_LONG || inst->type.type == VM_TYPE_POINTER))
			assert(!"can't bitcast structure to registers");

		SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
		GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[0]);

		unsigned index = 0;

		for(unsigned k = 0; k < sourceRegs.size(); k++)
		{
			if(!lowFunction->TransferRegisterTo(inst, sourceRegs[k]))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, index++, k + 1 == sourceRegs.size());

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, sourceRegs[k]);
			}
			else
			{
				index++;
			}
		}
	}
		break;
	case VM_INST_MOV:
	{
		SmallArray<unsigned char, 32> sourceRegs(ctx.allocator);
		GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[0]);

		for(unsigned k = 0; k < sourceRegs.size(); k++)
		{
			// We can never transfer source register to us, but since we should already have our target register allocated, it may match the source
			unsigned char copyReg = lowFunction->AllocateRegister(inst, k, k + 1 == sourceRegs.size());

			if(copyReg != sourceRegs[k])
			{
				assert(inst->type == inst->arguments[0]->type);

				VmValueType vmType = inst->type.type;
				RegVmCopyType copyType = rvcFull;

				if(vmType == VM_TYPE_INT || (vmType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4))
					copyType = rvcInt;
				else if(vmType == VM_TYPE_DOUBLE)
					copyType = rvcDouble;
				else if(vmType == VM_TYPE_LONG || (vmType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8))
					copyType = rvcLong;
				else
					assert(!"unknown type");

				if(RegVmLoweredInstruction *last = lowBlock->lastInstruction)
				{
					if(last->code == rviMov)
					{
						last->code = rviMovMult;
						last->rB |= copyType << 2;
						last->argument = CreateConstantInt(ctx.allocator, NULL, (copyReg << 24) | (sourceRegs[k] << 16));
					}
					else if(last->code == rviMovMult && (last->argument->iValue & 0xffff) == 0)
					{
						last->rB |= copyType << 4;
						last->argument->iValue |= (copyReg << 8) | sourceRegs[k];
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, (unsigned char)copyType, sourceRegs[k]);
					}
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, (unsigned char)copyType, sourceRegs[k]);
				}
			}
		}
	}
		break;
	case VM_INST_PHI:
		assert(!inst->regVmRegisters.empty());
		break;
	default:
		assert(!"unknown instruction");
	}

	if(RegVmLoweredInstruction *firstNewInstruction = lastLowered ? lastLowered->nextSibling : lowBlock->firstInstruction)
	{
		for(unsigned i = 0; i < lowFunction->killedRegisters.size(); i++)
			firstNewInstruction->preKillRegisters.push_back(lowFunction->killedRegisters[i]);
	}

	lowFunction->killedRegisters.clear();

	lowFunction->FreeConstantRegisters();
	lowFunction->FreeDelayedRegisters(lowBlock);
}

void CopyRegisters(VmInstruction *target, VmInstruction *source)
{
	assert(target->regVmRegisters.empty());

	for(unsigned i = 0; i < source->regVmRegisters.size(); i++)
		target->regVmRegisters.push_back(source->regVmRegisters[i]);

	target->regVmAllocated = true;
}

void CompareRegisters(VmInstruction *a, VmInstruction *b)
{
	(void)b;

	assert(a->regVmRegisters.size() == b->regVmRegisters.size());

	for(unsigned i = 0; i < a->regVmRegisters.size(); i++)
		assert(a->regVmRegisters[i] == b->regVmRegisters[i]);
}

void UpdateSharedStorage(VmInstruction *inst, unsigned marker)
{
	if(inst->regVmSearchMarker == marker)
		return;

	assert(!inst->regVmRegisters.empty());

	inst->regVmSearchMarker = marker;

	if(inst->cmd == VM_INST_PHI)
	{
		for(unsigned argumentPos = 0; argumentPos < inst->arguments.size(); argumentPos += 2)
		{
			VmInstruction *instruction = getType<VmInstruction>(inst->arguments[argumentPos]);

			if(!instruction->regVmRegisters.empty())
				CompareRegisters(instruction, inst);
			else
				CopyRegisters(instruction, inst);

			UpdateSharedStorage(instruction, marker);
		}
	}

	for(unsigned userPos = 0; userPos < inst->users.size(); userPos++)
	{
		VmInstruction *instruction = getType<VmInstruction>(inst->users[userPos]);

		if(instruction->cmd == VM_INST_PHI)
		{
			if(!instruction->regVmRegisters.empty())
				CompareRegisters(instruction, inst);
			else
				CopyRegisters(instruction, inst);

			UpdateSharedStorage(instruction, marker);
		}
	}
}

void CollectReservedRegisters(VmBlock *vmBlock, SmallArray<unsigned char, 16> &reservedRegisters, SmallArray<unsigned char, 16> &usedRegisters)
{
	for(unsigned i = 0; i < vmBlock->liveOut.size(); i++)
	{
		VmInstruction *liveOut = vmBlock->liveOut[i];

		if(!liveOut->regVmRegisters.empty())
		{
			for(unsigned k = 0; k < liveOut->regVmRegisters.size(); k++)
			{
				unsigned char reg = liveOut->regVmRegisters[k];

				if(!usedRegisters.contains(reg) && !reservedRegisters.contains(reg))
					reservedRegisters.push_back(reg);
			}
		}
	}

	for(unsigned i = 0; i < vmBlock->dominanceChildren.size(); i++)
	{
		VmBlock *curr = vmBlock->dominanceChildren[i];

		CollectReservedRegisters(curr, reservedRegisters, usedRegisters);
	}
}

void AllocateLiveInOutRegisters(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, VmBlock *vmBlock)
{
	// Check if live in has a color that is already allocated
	for(unsigned i = 0; i < vmBlock->liveIn.size(); i++)
	{
		VmInstruction *liveIn = vmBlock->liveIn[i];

		if(liveIn->regVmRegisters.empty() && liveIn->color != 0 && lowFunction->colorRegisters[liveIn->color])
		{
			VmInstruction *source = lowFunction->colorRegisters[liveIn->color];

			CopyRegisters(liveIn, source);

			UpdateSharedStorage(liveIn, lowFunction->vmFunction->nextSearchMarker++);
		}
	}

	// Check if live out has a color that is already allocated
	for(unsigned i = 0; i < vmBlock->liveOut.size(); i++)
	{
		VmInstruction *liveOut = vmBlock->liveOut[i];

		// Check if register color is already allocated
		if(liveOut->regVmRegisters.empty() && liveOut->color != 0 && lowFunction->colorRegisters[liveOut->color])
		{
			VmInstruction *source = lowFunction->colorRegisters[liveOut->color];

			CopyRegisters(liveOut, source);

			UpdateSharedStorage(liveOut, lowFunction->vmFunction->nextSearchMarker++);
		}
	}

	SmallArray<unsigned char, 16> usedRegisters(ctx.allocator);

	// Reserve registers already allocated by live in variables
	for(unsigned i = 0; i < vmBlock->liveIn.size(); i++)
	{
		VmInstruction *liveIn = vmBlock->liveIn[i];

		if(!liveIn->regVmRegisters.empty())
		{
			for(unsigned k = 0; k < liveIn->regVmRegisters.size(); k++)
			{
				unsigned char reg = liveIn->regVmRegisters[k];

				lowFunction->registerUsers[reg]++;

				usedRegisters.push_back(reg);
			}
		}
	}

	// Reserve registers already allocated by live out variables
	for(unsigned i = 0; i < vmBlock->liveOut.size(); i++)
	{
		VmInstruction *liveOut = vmBlock->liveOut[i];

		if(!liveOut->regVmRegisters.empty())
		{
			for(unsigned k = 0; k < liveOut->regVmRegisters.size(); k++)
			{
				unsigned char reg = liveOut->regVmRegisters[k];

				lowFunction->registerUsers[reg]++;

				usedRegisters.push_back(reg);
			}
		}
	}

	// Find which registers are already reserved in children nodes
	SmallArray<unsigned char, 16> reservedRegisters(ctx.allocator);

	for(unsigned i = 0; i < vmBlock->dominanceChildren.size(); i++)
	{
		VmBlock *curr = vmBlock->dominanceChildren[i];

		CollectReservedRegisters(curr, reservedRegisters, usedRegisters);
	}

	for(unsigned i = 0; i < reservedRegisters.size(); i++)
	{
		unsigned char reg = reservedRegisters[i];

		lowFunction->registerUsers[reg]++;
	}

	// Reset free registers state
	lowFunction->freedRegisters.clear();

	for(unsigned i = unsigned(lowFunction->nextRegister == 0 ? 255 : lowFunction->nextRegister) - 1; i >= rvrrCount; i--)
	{
		if(lowFunction->registerUsers[i] == 0)
			lowFunction->freedRegisters.push_back((unsigned char)i);
	}

	// Allocate exit registers for instructions that haven't been lowered yet
	for(unsigned i = 0; i < vmBlock->liveOut.size(); i++)
	{
		VmInstruction *liveOut = vmBlock->liveOut[i];

		if(liveOut->regVmRegisters.empty())
		{
			if(liveOut->color != 0)
				assert(lowFunction->colorRegisters[liveOut->color] == NULL);

			if(liveOut->type.type == VM_TYPE_INT || liveOut->type.type == VM_TYPE_LONG || liveOut->type.type == VM_TYPE_DOUBLE || liveOut->type.type == VM_TYPE_POINTER)
			{
				lowFunction->AllocateRegister(liveOut, 0, false);
			}
			else if(liveOut->type.type == VM_TYPE_FUNCTION_REF || liveOut->type.type == VM_TYPE_ARRAY_REF || liveOut->type.type == VM_TYPE_AUTO_REF)
			{
				lowFunction->AllocateRegister(liveOut, 0, false);
				lowFunction->AllocateRegister(liveOut, 1, false);
			}
			else if(liveOut->type.type == VM_TYPE_AUTO_ARRAY)
			{
				lowFunction->AllocateRegister(liveOut, 0, false);
				lowFunction->AllocateRegister(liveOut, 1, false);
				lowFunction->AllocateRegister(liveOut, 2, false);
			}
			else if(liveOut->type.type == VM_TYPE_STRUCT)
			{
				for(unsigned k = 0; k < (liveOut->type.size + 4) / 8; k++)
					lowFunction->AllocateRegister(liveOut, k, false);
			}

			liveOut->regVmAllocated = true;

			if(liveOut->color != 0)
				lowFunction->colorRegisters[liveOut->color] = liveOut;

			for(unsigned k = 0; k < liveOut->regVmRegisters.size(); k++)
				usedRegisters.push_back(liveOut->regVmRegisters[k]);

			UpdateSharedStorage(liveOut, lowFunction->vmFunction->nextSearchMarker++);
		}
	}

	for(unsigned i = 0; i < reservedRegisters.size(); i++)
	{
		unsigned char reg = reservedRegisters[i];

		assert(lowFunction->registerUsers[reg]);

		lowFunction->registerUsers[reg]--;
	}

	// Proceed to children in dominance tree
	for(unsigned i = 0; i < vmBlock->dominanceChildren.size(); i++)
	{
		VmBlock *curr = vmBlock->dominanceChildren[i];

		AllocateLiveInOutRegisters(ctx, lowFunction, curr);
	}

	// Clear register users
	for(unsigned i = 0; i < usedRegisters.size(); i++)
	{
		unsigned char reg = usedRegisters[i];

		assert(lowFunction->registerUsers[reg]);
		lowFunction->registerUsers[reg]--;

		// When last register use of a colored virtual register expires (stepping out of the block where it lives), reset mapping between color and physical register since a different assignment is possible
		if(lowFunction->registerUsers[reg] == 0)
		{
			for(unsigned colorPos = 0; colorPos < lowFunction->colorRegisters.size(); colorPos++)
			{
				if(VmInstruction *inst = lowFunction->colorRegisters[colorPos])
				{
					for(unsigned regPos = 0; regPos < inst->regVmRegisters.size(); regPos++)
					{
						if(inst->regVmRegisters[regPos] == reg)
						{
							lowFunction->colorRegisters[colorPos] = NULL;
							break;
						}
					}
				}
			}
		}
	}
}

RegVmLoweredBlock* RegVmLowerBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, VmBlock *vmBlock)
{
	RegVmLoweredBlock *lowBlock = new (ctx.get<RegVmLoweredBlock>()) RegVmLoweredBlock(ctx.allocator, lowFunction, vmBlock);

	lowFunction->blocks.push_back(lowBlock);

	// Get entry registers from instructions that already allocated them
	for(unsigned i = 0; i < vmBlock->liveIn.size(); i++)
	{
		VmInstruction *liveIn = vmBlock->liveIn[i];

		assert(!liveIn->regVmRegisters.empty());

		for(unsigned k = 0; k < liveIn->regVmRegisters.size(); k++)
		{
			unsigned char reg = liveIn->regVmRegisters[k];

			lowFunction->registerUsers[reg]++;

			assert(!lowBlock->entryRegisters.contains(reg));
			lowBlock->entryRegisters.push_back(reg);
		}
	}

	// Check if exit variable registers are already allocated
	for(unsigned i = 0; i < vmBlock->liveOut.size(); i++)
	{
		VmInstruction *liveOut = vmBlock->liveOut[i];

		assert(!liveOut->regVmRegisters.empty());

		for(unsigned k = 0; k < liveOut->regVmRegisters.size(); k++)
		{
			unsigned char reg = liveOut->regVmRegisters[k];

			lowFunction->registerUsers[reg]++;

			if(!lowBlock->reservedRegisters.contains(reg))
				lowBlock->reservedRegisters.push_back(reg);
		}
	}

	// Reset free registers state
	lowFunction->freedRegisters.clear();

	for(unsigned i = unsigned(lowFunction->nextRegister == 0 ? 255 : lowFunction->nextRegister) - 1; i >= rvrrCount; i--)
	{
		if(lowFunction->registerUsers[i] == 0)
			lowFunction->freedRegisters.push_back((unsigned char)i);
	}

	for(VmInstruction *vmInstruction = vmBlock->firstInstruction; vmInstruction; vmInstruction = vmInstruction->nextSibling)
	{
		LowerInstructionIntoBlock(ctx, lowFunction, lowBlock, vmInstruction);

		if(lowFunction->hasRegisterOverflow)
		{
			lowFunction->registerOverflowLocation = vmInstruction;

			return lowBlock;
		}
	}

	// Collect exit registers
	for(unsigned i = 0; i < vmBlock->liveOut.size(); i++)
	{
		VmInstruction *liveOut = vmBlock->liveOut[i];

		assert(!liveOut->regVmRegisters.empty());

		for(unsigned k = 0; k < liveOut->regVmRegisters.size(); k++)
		{
			unsigned char reg = liveOut->regVmRegisters[k];

			lowBlock->exitRegisters.push_back(reg);

			// Might not be the only use
			assert(lowFunction->registerUsers[reg] != 0);
			lowFunction->registerUsers[reg]--;

			if(lowFunction->registerUsers[reg] == 0)
			{
				assert(!lowFunction->freedRegisters.contains(reg));
				lowFunction->freedRegisters.push_back(reg);
			}
		}
	}

	// Free entry registers
	for(unsigned i = 0; i < vmBlock->liveIn.size(); i++)
	{
		VmInstruction *liveIn = vmBlock->liveIn[i];

		for(unsigned k = 0; k < liveIn->regVmRegisters.size(); k++)
		{
			unsigned char reg = liveIn->regVmRegisters[k];

			assert(lowFunction->registerUsers[reg] != 0);
			lowFunction->registerUsers[reg]--;

			if(lowFunction->registerUsers[reg] == 0)
			{
				assert(!lowFunction->freedRegisters.contains(reg));
				lowFunction->freedRegisters.push_back(reg);
			}
		}
	}

	for(unsigned i = 0; i < lowFunction->registerUsers.size(); i++)
		assert(lowFunction->registerUsers[i] == 0);

	return lowBlock;
}

RegVmLoweredFunction* RegVmLowerFunction(ExpressionContext &ctx, RegVmLoweredModule *lowModule, VmFunction *vmFunction)
{
	TRACE_SCOPE("InstructionTreeRegVmLower", "RegVmLowerFunction");

	if(vmFunction->function && vmFunction->function->name)
		TRACE_LABEL2(vmFunction->function->name->name.begin, vmFunction->function->name->name.end);

	RegVmLoweredFunction *lowFunction = new (ctx.get<RegVmLoweredFunction>()) RegVmLoweredFunction(ctx.allocator, lowModule, vmFunction);

	lowModule->functions.push_back(lowFunction);

	assert(vmFunction->firstBlock);

	lowFunction->colorRegisters.resize(vmFunction->nextColor + 1);
	memset(lowFunction->colorRegisters.data, 0, lowFunction->colorRegisters.size() * sizeof(lowFunction->colorRegisters[0]));

	AllocateLiveInOutRegisters(ctx, lowFunction, vmFunction->firstBlock);

	for(unsigned i = 0; i < lowFunction->colorRegisters.size(); i++)
		assert(lowFunction->colorRegisters[i] == NULL);

	for(unsigned i = 0; i < 256; i++)
		assert(lowFunction->registerUsers[i] == 0);

	for(VmBlock *vmBlock = vmFunction->firstBlock; vmBlock; vmBlock = vmBlock->nextSibling)
	{
		RegVmLowerBlock(ctx, lowFunction, vmBlock);

		if(lowFunction->hasRegisterOverflow)
			break;
	}

	return lowFunction;
}

RegVmLoweredModule* RegVmLowerModule(ExpressionContext &ctx, VmModule *vmModule)
{
	TRACE_SCOPE("InstructionTreeRegVmLower", "RegVmLowerModule");

	RegVmLoweredModule *lowModule = new (ctx.get<RegVmLoweredModule>()) RegVmLoweredModule(ctx.allocator, vmModule);

	for(VmFunction *vmFunction = vmModule->functions.head; vmFunction; vmFunction = vmFunction->next)
	{
		if(vmFunction->function && vmFunction->function->importModule != NULL)
			continue;

		if(vmFunction->function && vmFunction->function->isPrototype && !vmFunction->function->implementation)
			continue;

		RegVmLoweredFunction *lowFunction = RegVmLowerFunction(ctx, lowModule, vmFunction);

		if(lowFunction->hasRegisterOverflow)
			break;
	}

	return lowModule;
}

void RegFinalizeInstruction(InstructionRegVmFinalizeContext &ctx, RegVmLoweredInstruction *lowInstruction)
{
	ctx.locations.push_back(lowInstruction->location);

	RegVmCmd cmd;

	cmd.code = (unsigned char)lowInstruction->code;

	cmd.rA = lowInstruction->rA;
	cmd.rB = lowInstruction->rB;
	cmd.rC = lowInstruction->rC;

	if(VmConstant *argument = lowInstruction->argument)
	{
		if(VariableData *container = argument->container)
		{
			unsigned moduleId = container->importModule ? container->importModule->importIndex << 24 : 0;

			cmd.argument = argument->iValue + container->offset + moduleId;
		}
		else if(VmFunction *function = argument->fValue)
		{
			if(cmd.code == rviCall || cmd.code == rviFuncAddr)
			{
				FunctionData *data = function->function;

				cmd.argument = data->functionIndex;
			}
			else
			{
				assert(!"unknown instruction argument");
			}
		}
		else if(argument->bValue)
		{
			ctx.fixupPoints.push_back(InstructionRegVmFinalizeContext::FixupPoint(ctx.cmds.size(), argument->bValue));
			cmd.argument = ~0u;
		}
		else
		{
			cmd.argument = argument->iValue;
		}
	}

	ctx.cmds.push_back(cmd);

	// Register kill info
	unsigned preKillCount = lowInstruction->preKillRegisters.size();
	unsigned postKillCount = lowInstruction->postKillRegisters.size();

	if(preKillCount > 15)
		preKillCount = 15;

	if(postKillCount > 15)
		postKillCount = 15;

	ctx.regKillInfo.push_back((unsigned char)((preKillCount << 4) | postKillCount));

	for(unsigned i = 0; i < preKillCount; i++)
		ctx.regKillInfo.push_back(lowInstruction->preKillRegisters[i]);

	for(unsigned i = 0; i < postKillCount; i++)
		ctx.regKillInfo.push_back(lowInstruction->postKillRegisters[i]);
}

void RegFinalizeBlock(InstructionRegVmFinalizeContext &ctx, RegVmLoweredBlock *lowBlock)
{
	lowBlock->vmBlock->address = ctx.cmds.size();

	for(RegVmLoweredInstruction *curr = lowBlock->firstInstruction; curr; curr = curr->nextSibling)
	{
		RegFinalizeInstruction(ctx, curr);
	}
}

void RegFinalizeFunction(InstructionRegVmFinalizeContext &ctx, RegVmLoweredFunction *lowFunction)
{
	lowFunction->vmFunction->regVmAddress = ctx.cmds.size();
	lowFunction->vmFunction->regVmRegisters = lowFunction->nextRegister == 0 ? 256 : lowFunction->nextRegister;

	for(unsigned i = 0; i < lowFunction->blocks.size(); i++)
	{
		RegVmLoweredBlock *lowBlock = lowFunction->blocks[i];

		RegFinalizeBlock(ctx, lowBlock);
	}

	for(unsigned i = 0; i < ctx.fixupPoints.size(); i++)
	{
		InstructionRegVmFinalizeContext::FixupPoint &point = ctx.fixupPoints[i];

		assert(point.target);
		assert(point.target->address != ~0u);

		ctx.cmds[point.cmdIndex].argument = point.target->address;
	}

	ctx.fixupPoints.clear();

	lowFunction->vmFunction->regVmCodeSize = ctx.cmds.size() - lowFunction->vmFunction->regVmAddress;

	ctx.currentFunction = NULL;
}

void RegVmFinalizeModule(InstructionRegVmFinalizeContext &ctx, RegVmLoweredModule *lowModule)
{
	TRACE_SCOPE("InstructionTreeRegVmLower", "RegVmFinalizeModule");

	ctx.locations.push_back(NULL);
	ctx.cmds.push_back(RegVmCmd(rviJmp, 1, 0, 0, 0));
	ctx.regKillInfo.push_back(0);

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		RegVmLoweredFunction *lowFunction = lowModule->functions[i];

		if(!lowFunction->vmFunction->function)
		{
			lowModule->vmModule->regVmGlobalCodeStart = ctx.cmds.size();

			ctx.cmds[0].argument = lowModule->vmModule->regVmGlobalCodeStart;
		}

		RegFinalizeFunction(ctx, lowFunction);
	}

	ctx.constants.push_back(lowModule->constants.data, lowModule->constants.size());

	// Keep 8 byte alignment after module link
	if(ctx.constants.size() % 2 != 0)
		ctx.constants.push_back(0);
}
