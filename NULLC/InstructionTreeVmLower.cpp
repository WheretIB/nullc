#include "InstructionTreeVmLower.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, VmLoweredInstruction* instruction)
{
	assert(instruction);
	assert(instruction->parent == NULL);
	assert(instruction->prevSibling == NULL);
	assert(instruction->nextSibling == NULL);

	instruction->parent = this;

	instruction->stackDepthBefore = stackDepth;
	
	if(instruction->flag)
		assert(!instruction->flag->container);

	if(instruction->helper)
		assert(!instruction->helper->container);

	if(instruction->argument && instruction->argument->container)
		instruction->argument->container->lowUsers.push_back(instruction);

	// Update stack depth
	const unsigned pointerSlots = sizeof(void*) == 4 ? 1 : 2;

	switch(instruction->cmd)
	{
	case cmdNop:
		break;
	case cmdPushChar:
	case cmdPushShort:
	case cmdPushInt:
		stackDepth += 1;
		break;
	case cmdPushFloat:
	case cmdPushDorL:
		stackDepth += 2;
		break;
	case cmdPushCmplx:
		assert(instruction->helper);

		stackDepth += instruction->helper->iValue / 4;
		break;
	case cmdPushCharStk:
	case cmdPushShortStk:
	case cmdPushIntStk:
		stackDepth -= pointerSlots;
		stackDepth += 1;
		break;
	case cmdPushFloatStk:
	case cmdPushDorLStk:
		stackDepth -= pointerSlots;
		stackDepth += 2;
		break;
	case cmdPushCmplxStk:
		assert(instruction->helper);

		stackDepth -= pointerSlots;
		stackDepth += instruction->helper->iValue / 4;
		break;
	case cmdPushImmt:
		stackDepth += 1;
		break;
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
		break;
	case cmdMovCharStk:
	case cmdMovShortStk:
	case cmdMovIntStk:
	case cmdMovFloatStk:
	case cmdMovDorLStk:
	case cmdMovCmplxStk:
		stackDepth -= pointerSlots;
		break;
	case cmdPop:
		assert(instruction->argument);

		stackDepth -= instruction->argument->iValue / 4;
		break;
	case cmdDtoI:
		stackDepth -= 1;
		break;
	case cmdDtoL:
		break;
	case cmdDtoF:
		stackDepth -= 1;
		break;
	case cmdItoD:
		stackDepth += 1;
		break;
	case cmdLtoD:
		break;
	case cmdItoL:
		stackDepth += 1;
		break;
	case cmdLtoI:
		stackDepth -= 1;
		break;
	case cmdIndex:
		stackDepth -= 1;
		break;
	case cmdIndexStk:
		stackDepth -= 2;
		break;
	case cmdCopyDorL:
		stackDepth += 2;
		break;
	case cmdCopyI:
		stackDepth += 1;
		break;
	case cmdGetAddr:
		stackDepth += pointerSlots;
		break;
	case cmdFuncAddr:
		stackDepth += 1;
		break;
	case cmdSetRangeStk:
		stackDepth -= pointerSlots;
		break;
	case cmdJmp:
		break;
	case cmdJmpZ:
	case cmdJmpNZ:
		stackDepth -= 1;
		break;
	case cmdCall:
		assert(instruction->argument);

		if(FunctionData *function = instruction->argument->fValue->function)
		{
			stackDepth -= int(function->argumentsSize) / 4;

			if(function->type->returnType == ctx.typeFloat)
				stackDepth += 2;
			else
				stackDepth += int(function->type->returnType->size + 3) / 4;
		}
		else
		{
			assert(!"failed to find function");
		}
		break;
	case cmdCallPtr:
		assert(instruction->helper);
		assert(instruction->argument);

		stackDepth -= 1;
		stackDepth -= instruction->argument->iValue / 4;

		if(instruction->helper->iValue & bitRetSimple)
		{
			if((instruction->helper->iValue & 0xf) == OTYPE_INT)
				stackDepth += 1;
			else if((instruction->helper->iValue & 0xf) == OTYPE_DOUBLE)
				stackDepth += 2;
			else if((instruction->helper->iValue & 0xf) == OTYPE_LONG)
				stackDepth += 2;
		}
		else
		{
			stackDepth += instruction->helper->iValue / 4;
		}
		break;
	case cmdReturn:
		assert(instruction->argument);

		stackDepth -= instruction->argument->iValue / 4;
		break;
	case cmdYield:
		break;
	case cmdPushVTop:
		stackDepth += 1;
		break;
	case cmdAdd:
	case cmdSub:
	case cmdMul:
	case cmdDiv:
	case cmdPow:
	case cmdMod:
		stackDepth -= 1;
		break;
	case cmdLess:
	case cmdGreater:
	case cmdLEqual:
	case cmdGEqual:
	case cmdEqual:
	case cmdNEqual:
		stackDepth -= 1;
		break;
	case cmdShl:
	case cmdShr:
	case cmdBitAnd:
	case cmdBitOr:
	case cmdBitXor:
		stackDepth -= 1;
		break;
	case cmdLogAnd:
	case cmdLogOr:
	case cmdLogXor:
		stackDepth -= 1;
		break;
	case cmdAddL:
	case cmdSubL:
	case cmdMulL:
	case cmdDivL:
	case cmdPowL:
	case cmdModL:
		stackDepth -= 2;
		break;
	case cmdLessL:
	case cmdGreaterL:
	case cmdLEqualL:
	case cmdGEqualL:
	case cmdEqualL:
	case cmdNEqualL:
		stackDepth -= 3;
		break;
	case cmdShlL:
	case cmdShrL:
	case cmdBitAndL:
	case cmdBitOrL:
	case cmdBitXorL:
		stackDepth -= 2;
		break;
	case cmdLogAndL:
	case cmdLogOrL:
	case cmdLogXorL:
		stackDepth -= 3;
		break;
	case cmdAddD:
	case cmdSubD:
	case cmdMulD:
	case cmdDivD:
	case cmdPowD:
	case cmdModD:
		stackDepth -= 2;
		break;
	case cmdLessD:
	case cmdGreaterD:
	case cmdLEqualD:
	case cmdGEqualD:
	case cmdEqualD:
	case cmdNEqualD:
		stackDepth -= 3;
		break;
	case cmdNeg:
	case cmdNegL:
	case cmdNegD:
	case cmdBitNot:
	case cmdBitNotL:
		break;
	case cmdLogNot:
		break;
	case cmdLogNotL:
		stackDepth -= 1;
		break;
	case cmdIncI:
	case cmdIncD:
	case cmdIncL:
	case cmdDecI:
	case cmdDecD:
	case cmdDecL:
		break;
	case cmdPushTypeID:
		stackDepth += 1;
		break;
	case cmdConvertPtr:
		stackDepth -= 1;
		break;
	case cmdPushPtr:
		stackDepth += pointerSlots;
		break;
	case cmdPushPtrStk:
		break;
	case cmdPushPtrImmt:
		stackDepth += pointerSlots;
		break;
	case cmdCheckedRet:
		break;
	default:
		assert(!"unknown instruction");
	}

	assert(int(stackDepth) >= 0);

	instruction->stackDepthAfter = stackDepth;

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
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, NULL, NULL));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, NULL, argument));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, NULL, CreateConstantInt(ctx.allocator, NULL, argument)));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmBlock *argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, NULL, CreateConstantBlock(ctx.allocator, NULL, argument)));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmConstant *helper, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, helper, argument));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned short helper, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, CreateConstantInt(ctx.allocator, NULL, helper), argument));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned short helper, unsigned argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, NULL, CreateConstantInt(ctx.allocator, NULL, helper), CreateConstantInt(ctx.allocator, NULL, argument)));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmConstant *flag, VmConstant *helper, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, flag, helper, argument));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned char flag, unsigned short helper, unsigned argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, CreateConstantInt(ctx.allocator, NULL, flag), CreateConstantInt(ctx.allocator, NULL, helper), CreateConstantInt(ctx.allocator, NULL, argument)));
}

void VmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned char flag, unsigned short helper, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<VmLoweredInstruction>()) VmLoweredInstruction(location, cmd, CreateConstantInt(ctx.allocator, NULL, flag), CreateConstantInt(ctx.allocator, NULL, helper), argument));
}

void VmLoweredBlock::AttachInstructionAt(VmLoweredInstruction* instruction, VmLoweredInstruction* insertPoint)
{
	assert(instruction);
	assert(instruction->parent == NULL);
	assert(instruction->prevSibling == NULL);
	assert(instruction->nextSibling == NULL);

	instruction->parent = this;

	unsigned prevStackSize = insertPoint ? insertPoint->stackDepthAfter : 0;
	
	int stackSizeChange = int(instruction->stackDepthAfter) - int(instruction->stackDepthBefore);

	instruction->stackDepthBefore = prevStackSize;
	instruction->stackDepthAfter = prevStackSize + stackSizeChange;

	if(instruction->nextSibling)
		assert(instruction->nextSibling->stackDepthBefore == instruction->stackDepthAfter);

	if(!firstInstruction)
	{
		assert(!insertPoint);

		firstInstruction = lastInstruction = instruction;
	}
	else if(!insertPoint)
	{
		firstInstruction->prevSibling = instruction;

		instruction->nextSibling = firstInstruction;

		firstInstruction = instruction;
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
}

void VmLoweredBlock::DetachInstruction(VmLoweredInstruction* instruction)
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

void RemoveContainerUse(VariableData *container, VmLoweredInstruction* instruction)
{
	for(unsigned i = 0; i < container->lowUsers.size(); i++)
	{
		if(container->lowUsers[i] == instruction)
		{
			container->lowUsers[i] = container->lowUsers.back();
			container->lowUsers.pop_back();
			break;
		}
	}
}

void VmLoweredBlock::RemoveInstruction(VmLoweredInstruction* instruction)
{
	DetachInstruction(instruction);

	if(instruction->argument && instruction->argument->container)
		RemoveContainerUse(instruction->argument->container, instruction);
}

bool IsBlockTerminator(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdJmp:
	case cmdJmpZ:
	case cmdJmpNZ:
	case cmdReturn:
		return true;
	default:
		break;
	}

	return false;
}

bool HasMemoryWrite(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
	case cmdMovCharStk:
	case cmdMovShortStk:
	case cmdMovIntStk:
	case cmdMovFloatStk:
	case cmdMovDorLStk:
	case cmdMovCmplxStk:
	case cmdSetRangeStk:
	case cmdCall:
	case cmdCallPtr:
		return true;
	default:
		break;
	}

	return false;
}

bool HasMemoryAccess(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdPushChar:
	case cmdPushShort:
	case cmdPushInt:
	case cmdPushFloat:
	case cmdPushDorL:
	case cmdPushCmplx:
	case cmdPushCharStk:
	case cmdPushShortStk:
	case cmdPushIntStk:
	case cmdPushFloatStk:
	case cmdPushDorLStk:
	case cmdPushCmplxStk:
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
	case cmdMovCharStk:
	case cmdMovShortStk:
	case cmdMovIntStk:
	case cmdMovFloatStk:
	case cmdMovDorLStk:
	case cmdMovCmplxStk:
	case cmdSetRangeStk:
	case cmdCall:
	case cmdCallPtr:
		return true;
	default:
		break;
	}

	return false;
}

void CheckZeroOffset(VmValue *value)
{
	VmConstant *offset = getType<VmConstant>(value);

	(void)offset;
	assert(offset->iValue == 0);
}

void LowerIntoBlock(ExpressionContext &ctx, VmLoweredBlock *lowBlock, VmValue *value);

void LowerLoadIntoBlock(ExpressionContext &ctx, VmLoweredBlock *lowBlock, SynBase *source, VmInstructionType loadType, VmValue *address, VmValue *offset)
{
	VmConstant *constantOffset = getType<VmConstant>(offset);

	if(VmConstant *constant = getType<VmConstant>(address))
	{
		assert(constantOffset->iValue == 0);

		if(loadType == VM_INST_LOAD_BYTE)
			lowBlock->AddInstruction(ctx, source, cmdPushChar, IsLocalScope(constant->container->scope), 1, constant);
		else if(loadType == VM_INST_LOAD_SHORT)
			lowBlock->AddInstruction(ctx, source, cmdPushShort, IsLocalScope(constant->container->scope), 2, constant);
		else if(loadType == VM_INST_LOAD_INT)
			lowBlock->AddInstruction(ctx, source, cmdPushInt, IsLocalScope(constant->container->scope), 4, constant);
		else if(loadType == VM_INST_LOAD_FLOAT)
			lowBlock->AddInstruction(ctx, source, cmdPushFloat, IsLocalScope(constant->container->scope), 4, constant);
		else if(loadType == VM_INST_LOAD_DOUBLE || loadType == VM_INST_LOAD_LONG)
			lowBlock->AddInstruction(ctx, source, cmdPushDorL, IsLocalScope(constant->container->scope), 8, constant);
		else
			assert(!"unknown type");
	}
	else
	{
		LowerIntoBlock(ctx, lowBlock, address);

		if(loadType == VM_INST_LOAD_BYTE)
			lowBlock->AddInstruction(ctx, source, cmdPushCharStk, 1, constantOffset->iValue);
		else if(loadType == VM_INST_LOAD_SHORT)
			lowBlock->AddInstruction(ctx, source, cmdPushShortStk, 2, constantOffset->iValue);
		else if(loadType == VM_INST_LOAD_INT)
			lowBlock->AddInstruction(ctx, source, cmdPushIntStk, 4, constantOffset->iValue);
		else if(loadType == VM_INST_LOAD_FLOAT)
			lowBlock->AddInstruction(ctx, source, cmdPushFloatStk, 4, constantOffset->iValue);
		else if(loadType == VM_INST_LOAD_DOUBLE || loadType == VM_INST_LOAD_LONG)
			lowBlock->AddInstruction(ctx, source, cmdPushDorLStk, 8, constantOffset->iValue);
		else
			assert(!"unknown type");
	}
}

void LowerIntoBlock(ExpressionContext &ctx, VmLoweredBlock *lowBlock, VmValue *value)
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
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, inst->cmd, inst->arguments[0], inst->arguments[1]);
			break;
		case VM_INST_LOAD_STRUCT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				InstructionCode cmd = inst->type.size == 4 ? cmdPushInt : (inst->type.size == 8 ? cmdPushDorL : cmdPushCmplx);

				assert((unsigned short)inst->type.size == inst->type.size);

				lowBlock->AddInstruction(ctx, inst->source, cmd, IsLocalScope(constant->container->scope), (unsigned short)inst->type.size, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				InstructionCode cmd = inst->type.size == 4 ? cmdPushIntStk : (inst->type.size == 8 ? cmdPushDorLStk : cmdPushCmplxStk);

				assert((unsigned short)inst->type.size == inst->type.size);

				lowBlock->AddInstruction(ctx, inst->source, cmd, (unsigned short)inst->type.size, offset->iValue);
			}
			break;
		case VM_INST_LOAD_IMMEDIATE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			break;
		case VM_INST_STORE_BYTE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovChar, IsLocalScope(constant->container->scope), 1, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdMovCharStk, 1, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->arguments[2]->type.size);
			break;
		case VM_INST_STORE_SHORT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovShort, IsLocalScope(constant->container->scope), 2, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdMovShortStk, 2, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->arguments[2]->type.size);
			break;
		case VM_INST_STORE_INT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovInt, IsLocalScope(constant->container->scope), 4, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdMovIntStk, 4, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->arguments[2]->type.size);
			break;
		case VM_INST_STORE_FLOAT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovFloat, IsLocalScope(constant->container->scope), 4, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdMovFloatStk, 4, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->arguments[2]->type.size);
			break;
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_DOUBLE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovDorL, IsLocalScope(constant->container->scope), 8, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdMovDorLStk, 8, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->arguments[2]->type.size);
			break;
		case VM_INST_STORE_STRUCT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				assert((unsigned short)inst->arguments[2]->type.size == inst->arguments[2]->type.size);

				lowBlock->AddInstruction(ctx, inst->source, inst->arguments[2]->type.size == 8 ? cmdMovDorL : cmdMovCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->arguments[2]->type.size, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				assert((unsigned short)inst->arguments[2]->type.size == inst->arguments[2]->type.size);

				lowBlock->AddInstruction(ctx, inst->source, inst->arguments[2]->type.size == 8 ? cmdMovDorLStk : cmdMovCmplxStk, (unsigned short)inst->arguments[2]->type.size, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->arguments[2]->type.size);
			break;
		case VM_INST_DOUBLE_TO_INT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, cmdDtoI);
			break;
		case VM_INST_DOUBLE_TO_LONG:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, cmdDtoL);
			break;
		case VM_INST_DOUBLE_TO_FLOAT:
			if(VmConstant *argument = getType<VmConstant>(inst->arguments[0]))
			{
				float result = float(argument->dValue);

				unsigned target = 0;
				memcpy(&target, &result, sizeof(float));

				lowBlock->AddInstruction(ctx, inst->source, cmdPushImmt, target);
			}
			else
			{
				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				lowBlock->AddInstruction(ctx, inst->source, cmdDtoF);
			}
			break;
		case VM_INST_INT_TO_DOUBLE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, cmdItoD);
			break;
		case VM_INST_LONG_TO_DOUBLE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, cmdLtoD);
			break;
		case VM_INST_INT_TO_LONG:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, cmdItoL);
			break;
		case VM_INST_LONG_TO_INT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, cmdLtoI);
			break;
		case VM_INST_INDEX:
		{
			VmConstant *arrSize = getType<VmConstant>(inst->arguments[0]);
			VmConstant *elementSize = getType<VmConstant>(inst->arguments[1]);
			VmValue *pointer = inst->arguments[2];
			VmValue *index = inst->arguments[3];

			assert(arrSize && elementSize);

			LowerIntoBlock(ctx, lowBlock, pointer);
			LowerIntoBlock(ctx, lowBlock, index);

			assert((unsigned short)elementSize->iValue == elementSize->iValue);

			lowBlock->AddInstruction(ctx, inst->source, cmdIndex, (unsigned short)elementSize->iValue, arrSize->iValue);
		}
		break;
		case VM_INST_INDEX_UNSIZED:
		{
			VmConstant *elementSize = getType<VmConstant>(inst->arguments[0]);
			VmValue *arr = inst->arguments[1];
			VmValue *index = inst->arguments[2];

			assert(elementSize);

			LowerIntoBlock(ctx, lowBlock, arr);
			LowerIntoBlock(ctx, lowBlock, index);

			assert((unsigned short)elementSize->iValue == elementSize->iValue);

			lowBlock->AddInstruction(ctx, inst->source, cmdIndexStk, (unsigned short)elementSize->iValue, 0u);
		}
		break;
		case VM_INST_FUNCTION_ADDRESS:
		{
			VmConstant *funcIndex = getType<VmConstant>(inst->arguments[0]);

			assert(funcIndex);

			lowBlock->AddInstruction(ctx, inst->source, cmdFuncAddr, funcIndex);
		}
		break;
		case VM_INST_TYPE_ID:
		{
			VmConstant *typeIndex = getType<VmConstant>(inst->arguments[0]);

			assert(typeIndex);

			lowBlock->AddInstruction(ctx, inst->source, cmdPushTypeID, typeIndex->iValue);
		}
		break;
		case VM_INST_SET_RANGE:
		{
			VmValue *address = inst->arguments[0];
			VmConstant *count = getType<VmConstant>(inst->arguments[1]);
			VmValue *initializer = inst->arguments[2];
			VmConstant *elementSize = getType<VmConstant>(inst->arguments[3]);

			LowerIntoBlock(ctx, lowBlock, initializer);
			LowerIntoBlock(ctx, lowBlock, address);

			if(initializer->type == VmType::Int && elementSize->iValue == 1)
				lowBlock->AddInstruction(ctx, inst->source, cmdSetRangeStk, DTYPE_CHAR, count->iValue);
			else if(initializer->type == VmType::Int && elementSize->iValue == 2)
				lowBlock->AddInstruction(ctx, inst->source, cmdSetRangeStk, DTYPE_SHORT, count->iValue);
			else if(initializer->type == VmType::Int && elementSize->iValue == 4)
				lowBlock->AddInstruction(ctx, inst->source, cmdSetRangeStk, DTYPE_INT, count->iValue);
			else if(initializer->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdSetRangeStk, DTYPE_LONG, count->iValue);
			else if(initializer->type == VmType::Double && elementSize->iValue == 4)
				lowBlock->AddInstruction(ctx, inst->source, cmdSetRangeStk, DTYPE_FLOAT, count->iValue);
			else if(initializer->type == VmType::Double && elementSize->iValue == 8)
				lowBlock->AddInstruction(ctx, inst->source, cmdSetRangeStk, DTYPE_DOUBLE, count->iValue);

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, initializer->type.size);
		}
			break;
		case VM_INST_MEM_COPY:
		{
			VmConstant *size = getType<VmConstant>(inst->arguments[4]);

			assert((unsigned short)size->iValue == size->iValue);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[2]))
			{
				CheckZeroOffset(inst->arguments[3]);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)size->iValue, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[3]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushCmplxStk, (unsigned short)size->iValue, offset->iValue);
			}

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				CheckZeroOffset(inst->arguments[1]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovCmplx, IsLocalScope(constant->container->scope), (unsigned short)size->iValue, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovCmplxStk, (unsigned short)size->iValue, offset->iValue);
			}

			lowBlock->AddInstruction(ctx, inst->source, cmdPop, size->iValue);
		}
			break;
		case VM_INST_JUMP:
			// Check if jump is fall-through
			if(!(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[0]))
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdJmp, getType<VmBlock>(inst->arguments[0]));
			}
			break;
		case VM_INST_JUMP_Z:
			assert(inst->arguments[0]->type.size == 4);

			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[1])
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdJmpNZ, getType<VmBlock>(inst->arguments[2]));
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdJmpZ, getType<VmBlock>(inst->arguments[1]));

				lowBlock->AddInstruction(ctx, inst->source, cmdJmp, getType<VmBlock>(inst->arguments[2]));
			}
			break;
		case VM_INST_JUMP_NZ:
			assert(inst->arguments[0]->type.size == 4);

			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[1])
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdJmpZ, getType<VmBlock>(inst->arguments[2]));
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdJmpNZ, getType<VmBlock>(inst->arguments[1]));

				lowBlock->AddInstruction(ctx, inst->source, cmdJmp, getType<VmBlock>(inst->arguments[2]));
			}
			break;
		case VM_INST_CALL:
		{
			assert((unsigned short)inst->type.size == inst->type.size);

			unsigned short helper = (unsigned short)inst->type.size;

			// Special cases for simple types
			if(inst->type == VmType::Int)
				helper = bitRetSimple | OTYPE_INT;
			else if(inst->type == VmType::Double)
				helper = bitRetSimple | OTYPE_DOUBLE;
			else if(inst->type == VmType::Long)
				helper = bitRetSimple | OTYPE_LONG;

			VmConstant *resultAddress = NULL;

			if(inst->arguments[0]->type.type == VM_TYPE_FUNCTION_REF)
			{
				unsigned paramSize = NULLC_PTR_SIZE;

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				if(VmInstruction *result = getType<VmInstruction>(inst->arguments[1]))
				{
					assert(result->cmd == VM_INST_REFERENCE);

					resultAddress = getType<VmConstant>(result->arguments[0]);
				}

				for(int i = int(inst->arguments.size() - 1); i >= 2; i--)
				{
					VmInstruction *argumentInst = getType<VmInstruction>(inst->arguments[i]);

					if(argumentInst && argumentInst->cmd == VM_INST_REFERENCE)
					{
						VmConstant *constant = getType<VmConstant>(argumentInst->arguments[0]);

						assert(constant->iValue == 0);

						lowBlock->AddInstruction(ctx, inst->source, cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)constant->container->type->size, constant);
					}
					else
					{
						LowerIntoBlock(ctx, lowBlock, inst->arguments[i]);
					}

					if(inst->arguments[i]->type.size == 0)
						lowBlock->AddInstruction(ctx, inst->arguments[i]->source, cmdPushImmt, 0u);

					unsigned size = inst->arguments[i]->type.size;

					paramSize += size > 4 ? size : 4;
				}

				lowBlock->AddInstruction(ctx, inst->source, cmdCallPtr, helper, paramSize);
			}
			else
			{
				VmValue *context = inst->arguments[0];
				VmFunction *function = getType<VmFunction>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, context);

				if(VmInstruction *result = getType<VmInstruction>(inst->arguments[2]))
				{
					assert(result->cmd == VM_INST_REFERENCE);

					resultAddress = getType<VmConstant>(result->arguments[0]);
				}

				for(int i = int(inst->arguments.size() - 1); i >= 3; i--)
				{
					VmInstruction *argumentInst = getType<VmInstruction>(inst->arguments[i]);

					if(argumentInst && argumentInst->cmd == VM_INST_REFERENCE)
					{
						VmConstant *constant = getType<VmConstant>(argumentInst->arguments[0]);

						assert(constant->iValue == 0);

						lowBlock->AddInstruction(ctx, inst->source, cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)constant->container->type->size, constant);
					}
					else
					{
						LowerIntoBlock(ctx, lowBlock, inst->arguments[i]);
					}

					if(inst->arguments[i]->type.size == 0)
						lowBlock->AddInstruction(ctx, inst->arguments[i]->source, cmdPushImmt, 0u);
				}

				lowBlock->AddInstruction(ctx, inst->source, cmdCall, helper, CreateConstantFunction(ctx.allocator, NULL, function));
			}

			if(resultAddress)
			{
				assert(resultAddress->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, cmdMovCmplx, IsLocalScope(resultAddress->container->scope), (unsigned short)resultAddress->container->type->size, resultAddress);
			}
		}
		break;
		case VM_INST_RETURN:
		{
			bool localReturn = lowBlock->vmBlock->parent->function != NULL;

			if(!inst->arguments.empty())
			{
				VmValue *result = inst->arguments[0];

				VmInstruction *resultInst = getType<VmInstruction>(result);

				if(resultInst && resultInst->cmd == VM_INST_REFERENCE)
				{
					VmConstant *constant = getType<VmConstant>(resultInst->arguments[0]);

					assert(constant->iValue == 0);

					lowBlock->AddInstruction(ctx, inst->source, cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)constant->container->type->size, constant);
				}
				else
				{
					if(result->type.size != 0)
						LowerIntoBlock(ctx, lowBlock, result);
				}

				unsigned char operType = OTYPE_COMPLEX;

				if(result->type == VmType::Int)
					operType = OTYPE_INT;
				else if(result->type == VmType::Double)
					operType = OTYPE_DOUBLE;
				else if(result->type == VmType::Long)
					operType = OTYPE_LONG;

				if(result->type.structType && (isType<TypeRef>(result->type.structType) || isType<TypeUnsizedArray>(result->type.structType)))
					lowBlock->AddInstruction(ctx, inst->source, cmdCheckedRet, result->type.structType->typeIndex);

				lowBlock->AddInstruction(ctx, inst->source, cmdReturn, operType, (unsigned short)localReturn, result->type.size);
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdReturn, 0, (unsigned short)localReturn, 0u);
			}
		}
		break;
		case VM_INST_YIELD:
		{
			if(!inst->arguments.empty())
			{
				VmValue *result = inst->arguments[0];

				if(result->type.size != 0)
					LowerIntoBlock(ctx, lowBlock, result);

				unsigned char operType = OTYPE_COMPLEX;

				if(result->type == VmType::Int)
					operType = OTYPE_INT;
				else if(result->type == VmType::Double)
					operType = OTYPE_DOUBLE;
				else if(result->type == VmType::Long)
					operType = OTYPE_LONG;

				if(result->type.structType && (isType<TypeRef>(result->type.structType) || isType<TypeUnsizedArray>(result->type.structType)))
					lowBlock->AddInstruction(ctx, inst->source, cmdCheckedRet, result->type.structType->typeIndex);

				lowBlock->AddInstruction(ctx, inst->source, cmdReturn, operType, 1, result->type.size);
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdReturn, 0, 1, 0u);
			}
		}
		break;
		case VM_INST_ADD:
		{
			bool isContantOneLhs = DoesConstantMatchEither(inst->arguments[0], 1, 1.0f, 1ll);

			if(isContantOneLhs || DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
			{
				LowerIntoBlock(ctx, lowBlock, isContantOneLhs ? inst->arguments[1] : inst->arguments[0]);

				if(inst->type == VmType::Int)
					lowBlock->AddInstruction(ctx, inst->source, cmdIncI);
				else if(inst->type == VmType::Double)
					lowBlock->AddInstruction(ctx, inst->source, cmdIncD);
				else if(inst->type == VmType::Long)
					lowBlock->AddInstruction(ctx, inst->source, cmdIncL);
				else
					assert(!"unknown type");
			}
			else
			{
				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

				if(inst->type == VmType::Int)
					lowBlock->AddInstruction(ctx, inst->source, cmdAdd);
				else if(inst->type == VmType::Double)
					lowBlock->AddInstruction(ctx, inst->source, cmdAddD);
				else if(inst->type == VmType::Long)
					lowBlock->AddInstruction(ctx, inst->source, cmdAddL);
				else if(inst->type.type == VM_TYPE_POINTER)
					lowBlock->AddInstruction(ctx, inst->source, cmdAdd); // TODO: what if the address is on a 32 bit value border
				else
					assert(!"unknown type");
			}
		}
		break;
		case VM_INST_SUB:
		{
			if(DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
			{
				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				if(inst->type == VmType::Int)
					lowBlock->AddInstruction(ctx, inst->source, cmdDecI);
				else if(inst->type == VmType::Double)
					lowBlock->AddInstruction(ctx, inst->source, cmdDecD);
				else if(inst->type == VmType::Long)
					lowBlock->AddInstruction(ctx, inst->source, cmdDecL);
				else
					assert(!"unknown type");
			}
			else
			{
				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

				if(inst->type == VmType::Int)
					lowBlock->AddInstruction(ctx, inst->source, cmdSub);
				else if(inst->type == VmType::Double)
					lowBlock->AddInstruction(ctx, inst->source, cmdSubD);
				else if(inst->type == VmType::Long)
					lowBlock->AddInstruction(ctx, inst->source, cmdSubL);
				else
					assert(!"unknown type");
			}
		}
		break;
		case VM_INST_MUL:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdMul);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdMulD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdMulL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_DIV:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdDiv);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdDivD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdDivL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_POW:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdPow);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdPowD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdPowL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_MOD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdMod);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdModD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdModL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LESS:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLess);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdLessD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLessL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_GREATER:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdGreater);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdGreaterD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdGreaterL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LESS_EQUAL:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdLEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_GREATER_EQUAL:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdGEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdGEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdGEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_EQUAL:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdEqualL);
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				lowBlock->AddInstruction(ctx, inst->source, sizeof(void*) == 4 ? cmdEqual : cmdEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_NOT_EQUAL:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdNEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdNEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdNEqualL);
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				lowBlock->AddInstruction(ctx, inst->source, sizeof(void*) == 4 ? cmdNEqual : cmdNEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_SHL:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdShl);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdShlL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_SHR:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdShr);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdShrL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_AND:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitAnd);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitAndL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_OR:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitOr);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitOrL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_XOR:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitXor);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitXorL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LOG_XOR:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerIntoBlock(ctx, lowBlock, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLogXor);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLogXorL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_ADD_LOAD:
		{
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdAdd);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdAddD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdAddL);
			else if(inst->type.type == VM_TYPE_POINTER)
				lowBlock->AddInstruction(ctx, inst->source, cmdAdd); // TODO: what if the address is on a 32 bit value border
			else
				assert(!"unknown type");
		}
		break;
		case VM_INST_SUB_LOAD:
		{
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdSub);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdSubD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdSubL);
			else
				assert(!"unknown type");
		}
		break;
		case VM_INST_MUL_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdMul);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdMulD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdMulL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_DIV_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdDiv);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdDivD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdDivL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_POW_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdPow);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdPowD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdPowL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_MOD_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdMod);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdModD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdModL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LESS_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLess);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdLessD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLessL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_GREATER_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdGreater);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdGreaterD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdGreaterL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LESS_EQUAL_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdLEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_GREATER_EQUAL_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdGEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdGEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdGEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_EQUAL_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdEqualL);
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				lowBlock->AddInstruction(ctx, inst->source, sizeof(void*) == 4 ? cmdEqual : cmdEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_NOT_EQUAL_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdNEqual);
			else if(inst->arguments[0]->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdNEqualD);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdNEqualL);
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				lowBlock->AddInstruction(ctx, inst->source, sizeof(void*) == 4 ? cmdNEqual : cmdNEqualL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_SHL_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdShl);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdShlL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_SHR_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdShr);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdShrL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_AND_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitAnd);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitAndL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_OR_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitOr);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitOrL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_XOR_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitXor);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitXorL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LOG_XOR_LOAD:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			LowerLoadIntoBlock(ctx, lowBlock, inst->source, VmInstructionType(getType<VmConstant>(inst->arguments[3])->iValue), inst->arguments[1], inst->arguments[2]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLogXor);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLogXorL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_NEG:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdNeg);
			else if(inst->type == VmType::Double)
				lowBlock->AddInstruction(ctx, inst->source, cmdNegD);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdNegL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_NOT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			if(inst->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitNot);
			else if(inst->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdBitNotL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_LOG_NOT:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

			if(inst->arguments[0]->type == VmType::Int)
				lowBlock->AddInstruction(ctx, inst->source, cmdLogNot);
			else if(inst->arguments[0]->type == VmType::Long)
				lowBlock->AddInstruction(ctx, inst->source, cmdLogNotL);
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				lowBlock->AddInstruction(ctx, inst->source, sizeof(void*) == 4 ? cmdLogNot : cmdLogNotL);
			else
				assert(!"unknown type");
			break;
		case VM_INST_CONVERT_POINTER:
		{
			VmValue *pointer = inst->arguments[0];
			VmConstant *typeIndex = getType<VmConstant>(inst->arguments[1]);

			assert(typeIndex);

			LowerIntoBlock(ctx, lowBlock, pointer);

			lowBlock->AddInstruction(ctx, inst->source, cmdConvertPtr, typeIndex->iValue);
		}
		break;
		case VM_INST_ABORT_NO_RETURN:
			lowBlock->AddInstruction(ctx, inst->source, cmdReturn, bitRetError, 1, 0u);
			break;
		case VM_INST_CONSTRUCT:
		case VM_INST_ARRAY:
			for(int i = int(inst->arguments.size() - 1); i >= 0; i--)
			{
				VmValue *argument = inst->arguments[i];

				assert(argument->type.size % 4 == 0);

				if(VmFunction *function = getType<VmFunction>(argument))
					lowBlock->AddInstruction(ctx, inst->source, cmdFuncAddr, function->function->functionIndex);
				else
					LowerIntoBlock(ctx, lowBlock, argument);
			}
			break;
		case VM_INST_EXTRACT:
			assert(!"invalid instruction");
			break;
		case VM_INST_UNYIELD:
			// Check secondary blocks first
			for(unsigned i = 2; i < inst->arguments.size(); i++)
			{
				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushImmt, i - 1);
				lowBlock->AddInstruction(ctx, inst->source, cmdEqual);

				lowBlock->AddInstruction(ctx, inst->source, cmdJmpNZ, getType<VmBlock>(inst->arguments[i]));
			}

			// jump to entry block by default
			if(!(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[1]))
			{
				lowBlock->AddInstruction(ctx, inst->source, cmdJmp, getType<VmBlock>(inst->arguments[1]));
			}
			break;
		case VM_INST_BITCAST:
		case VM_INST_MOV:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			break;
		case VM_INST_REFERENCE:
			break; // Handle in call and return instructions
		default:
			assert(!"unknown instruction");
		}

		if(inst->type.size != 0 && inst->users.empty())
			lowBlock->AddInstruction(ctx, inst->source, cmdPop, inst->type.size);
	}
	else if(VmConstant *constant = getType<VmConstant>(value))
	{
		if(constant->type == VmType::Void)
		{
			return;
		}
		else if(constant->type == VmType::Int)
		{
			lowBlock->AddInstruction(ctx, constant->source, cmdPushImmt, constant->iValue);
		}
		else if(constant->type == VmType::Double)
		{
			unsigned data[2];
			memcpy(data, &constant->dValue, 8);

			lowBlock->AddInstruction(ctx, constant->source, cmdPushImmt, data[1]);
			lowBlock->AddInstruction(ctx, constant->source, cmdPushImmt, data[0]);
		}
		else if(constant->type == VmType::Long)
		{
			unsigned data[2];
			memcpy(data, &constant->lValue, 8);

			lowBlock->AddInstruction(ctx, constant->source, cmdPushImmt, data[1]);
			lowBlock->AddInstruction(ctx, constant->source, cmdPushImmt, data[0]);
		}
		else if(constant->type.type == VM_TYPE_POINTER)
		{
			if(!constant->container)
			{
				assert(constant->iValue == 0);

				lowBlock->AddInstruction(ctx, constant->source, cmdPushPtrImmt, 0u);
			}
			else
			{
				lowBlock->AddInstruction(ctx, constant->source, cmdGetAddr, 0, IsLocalScope(constant->container->scope), constant);
			}
		}
		else if(constant->type.type == VM_TYPE_STRUCT)
		{
			assert(constant->type.size % 4 == 0);

			for(int i = int(constant->type.size / 4) - 1; i >= 0; i--)
				lowBlock->AddInstruction(ctx, constant->source, cmdPushImmt, ((unsigned*)constant->sValue)[i]);
		}
		else
		{
			assert(!"unknown type");
		}
	}
	else
	{
		assert(!"unknown type");
	}
}

VmLoweredBlock* LowerBlock(ExpressionContext &ctx, VmBlock *vmBlock)
{
	VmLoweredBlock *lowBlock = new (ctx.get<VmLoweredBlock>()) VmLoweredBlock(vmBlock);

	for(VmInstruction *vmInstruction = vmBlock->firstInstruction; vmInstruction; vmInstruction = vmInstruction->nextSibling)
	{
		if(!vmInstruction->users.empty())
			continue;

		LowerIntoBlock(ctx, lowBlock, vmInstruction);
	}

	return lowBlock;
}

VmLoweredFunction* LowerFunction(ExpressionContext &ctx, VmFunction *vmFunction)
{
	VmLoweredFunction *lowFunction = new (ctx.get<VmLoweredFunction>()) VmLoweredFunction(ctx.allocator, vmFunction);

	assert(vmFunction->firstBlock);

	for(VmBlock *vmBlock = vmFunction->firstBlock; vmBlock; vmBlock = vmBlock->nextSibling)
	{
		lowFunction->blocks.push_back(LowerBlock(ctx, vmBlock));
	}

	return lowFunction;
}

VmLoweredModule* LowerModule(ExpressionContext &ctx, VmModule *vmModule)
{
	TRACE_SCOPE("InstructionTreeVmLower", "LowerModule");

	VmLoweredModule *lowModule = new (ctx.get<VmLoweredModule>()) VmLoweredModule(ctx.allocator, vmModule);

	for(VmFunction *vmFunction = vmModule->functions.head; vmFunction; vmFunction = vmFunction->next)
	{
		if(vmFunction->function && vmFunction->function->importModule != NULL)
			continue;

		if(vmFunction->function && vmFunction->function->isPrototype && !vmFunction->function->implementation)
			continue;

		lowModule->functions.push_back(LowerFunction(ctx, vmFunction));
	}

	return lowModule;
}

bool IsSpillLoad(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdPushChar:
	case cmdPushShort:
	case cmdPushInt:
	case cmdPushFloat:
	case cmdPushDorL:
	case cmdPushCmplx:
		return lowInstruction->argument->container->isVmRegSpill;
	default:
		break;
	}

	return false;
}

bool IsSpillStore(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
		return lowInstruction->argument->container->isVmRegSpill;
	default:
		break;
	}

	return false;
}

bool IsVariableLoadOfSize(VmLoweredInstruction *lowInstruction, int size)
{
	switch(lowInstruction->cmd)
	{
	case cmdPushChar:
	case cmdPushShort:
	case cmdPushInt:
	case cmdPushFloat:
	case cmdPushDorL:
	case cmdPushCmplx:
		return lowInstruction->argument->container && lowInstruction->helper->iValue == size;
	default:
		break;
	}

	return false;
}

bool HasMemoryWriteToVariable(VmLoweredInstruction *lowInstruction, VariableData *variable)
{
	switch(lowInstruction->cmd)
	{
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
		return lowInstruction->argument->container == variable;
	default:
		break;
	}

	return false;
}

bool HasUnknownMemoryWrite(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdMovCharStk:
	case cmdMovShortStk:
	case cmdMovIntStk:
	case cmdMovFloatStk:
	case cmdMovDorLStk:
	case cmdMovCmplxStk:
	case cmdSetRangeStk:
	case cmdCall:
	case cmdCallPtr:
		return true;
	default:
		break;
	}

	return false;
}

VariableData* GetMemoryWriteOfAlloca(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
		if(!HasAddressTaken(lowInstruction->argument->container))
			return lowInstruction->argument->container;

		break;
	default:
		break;
	}

	return NULL;
}

VariableData* GetMemoryReadOfAlloca(VmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->cmd)
	{
	case cmdPushChar:
	case cmdPushShort:
	case cmdPushInt:
	case cmdPushFloat:
	case cmdPushDorL:
	case cmdPushCmplx:
		if(!HasAddressTaken(lowInstruction->argument->container))
			return lowInstruction->argument->container;

		break;
	default:
		break;
	}

	return NULL;
}

bool IsVariableStoreOfSize(VmLoweredInstruction *lowInstruction, int size)
{
	switch(lowInstruction->cmd)
	{
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
		return lowInstruction->argument->container && lowInstruction->helper->iValue == size;
	default:
		break;
	}

	return false;
}
bool HasOtherBlockUse(VariableData *spillVariable, VmLoweredBlock *lowBlock)
{
	for(unsigned i = 0; i < spillVariable->lowUsers.size(); i++)
	{
		if(spillVariable->lowUsers[i]->parent != lowBlock)
			return true;
	}

	return false;
}

void OptimizeByReadingTargetInsteadOfTemporary(VmLoweredModule *lowModule, VmLoweredBlock *lowBlock)
{
	(void)lowModule;

	// Go from last instruction back to first
	for(VmLoweredInstruction *curr = lowBlock->lastInstruction; curr;)
	{
		// We are looking for register spill loads
		if(!IsSpillLoad(curr))
		{
			curr = curr->prevSibling;
			continue;
		}

		// Spill variable
		VariableData *spillVariable = curr->argument->container;

		// For safety, check that all register spill users are from the current block
		if(HasOtherBlockUse(spillVariable, lowBlock))
		{
			curr = curr->prevSibling;
			continue;
		}

		VmLoweredInstruction *currPrevSibling = curr->prevSibling;

		if(spillVariable->lowUsers.size() >= 2)
		{
			assert(IsSpillStore(spillVariable->lowUsers[0]));

			VmLoweredInstruction *targetStore = spillVariable->lowUsers[0];

			// Check that target store received its data from a variable read of the same size
			if(IsVariableLoadOfSize(targetStore->prevSibling, targetStore->helper->iValue))
			{
				VmLoweredInstruction *originalLoad = targetStore->prevSibling;

				VariableData *originalVariable = originalLoad->argument->container;

				// Look if it's safe to move original read near the spilled register read
				VmLoweredInstruction *it = curr->prevSibling;

				bool safeToMove = true;

				while(it != targetStore)
				{
					// Depending on original variable usage type, we might ignore more side-effecting operations
					if(HasAddressTaken(originalVariable))
					{
						if(HasMemoryWriteToVariable(it, originalVariable))
						{
							safeToMove = false;
							break;
						}

						if(HasUnknownMemoryWrite(it))
						{
							safeToMove = false;
							break;
						}
					}
					else
					{
						if(HasMemoryWriteToVariable(it, originalVariable))
						{
							safeToMove = false;
							break;
						}
					}

					it = it->prevSibling;
				}

				if(safeToMove)
				{
					assert(curr->cmd == originalLoad->cmd);
					assert(curr->helper->iValue == originalLoad->helper->iValue);

					RemoveContainerUse(curr->argument->container, curr);

					curr->flag = originalLoad->flag;
					curr->argument = originalLoad->argument;

					curr->argument->container->lowUsers.push_back(curr);
				}
			}
		}

		curr = currPrevSibling;
	}
}

void OptimizeUnusedTemporaries(VmLoweredModule *lowModule, VmLoweredBlock *lowBlock)
{
	for(VmLoweredInstruction *curr = lowBlock->firstInstruction; curr;)
	{
		VmLoweredInstruction *next = curr->nextSibling;

		if(!IsSpillStore(curr))
		{
			curr = next;
			continue;
		}

		// Spill variable
		VariableData *spillVariable = curr->argument->container;

		if(spillVariable->lowUsers.size() == 1)
		{
			VmLoweredInstruction *targetStore = spillVariable->lowUsers[0];

			// Check that target store received its data from a variable read of the same size
			if(IsVariableLoadOfSize(targetStore->prevSibling, targetStore->helper->iValue))
			{
				VmLoweredInstruction *originalLoad = targetStore->prevSibling;

				// Check that target store data is not used after it
				if(targetStore->nextSibling->cmd == cmdPop && targetStore->nextSibling->argument->iValue == targetStore->helper->iValue)
				{
					VmLoweredInstruction *originalPop = targetStore->nextSibling;

					next = originalPop->nextSibling;

					// Detach original instructions
					lowBlock->RemoveInstruction(originalLoad);
					lowBlock->RemoveInstruction(targetStore);
					lowBlock->RemoveInstruction(originalPop);

					lowModule->removedSpilledRegisters++;
				}
			}
			else if(IsVariableStoreOfSize(targetStore->nextSibling, targetStore->helper->iValue))
			{
				lowBlock->RemoveInstruction(targetStore);

				lowModule->removedSpilledRegisters++;
			}
		}

		curr = next;
	}
}

void OptimizeSingleImmediateTemporaryUses(VmLoweredModule *lowModule, VmLoweredBlock *lowBlock)
{
	// Go from last instruction back to first
	for(VmLoweredInstruction *curr = lowBlock->lastInstruction; curr;)
	{
		// We are looking for register spill loads
		if(!IsSpillLoad(curr))
		{
			curr = curr->prevSibling;
			continue;
		}

		// Spill variable
		VariableData *spillVariable = curr->argument->container;

		// For safety, check that all register spill users are from the current block
		if(HasOtherBlockUse(spillVariable, lowBlock))
		{
			curr = curr->prevSibling;
			continue;
		}

		VmLoweredInstruction *currPrevSibling = curr->prevSibling;

		// If we find a spill read with only a single other use (should be a store)
		if(spillVariable->lowUsers.size() == 2)
		{
			assert(IsSpillStore(spillVariable->lowUsers[0]));

			VmLoweredInstruction *targetStore = spillVariable->lowUsers[0];

			// Check if the single use is not required at all
			if(targetStore->nextSibling->nextSibling == curr)
			{
				// Check that target store data is not used after it
				if(targetStore->nextSibling->cmd == cmdPop && targetStore->nextSibling->argument->iValue == targetStore->helper->iValue)
				{
					VmLoweredInstruction *originalPop = targetStore->nextSibling;

					// Detach original instructions
					lowBlock->RemoveInstruction(targetStore);
					lowBlock->RemoveInstruction(originalPop);

					currPrevSibling = curr->prevSibling;

					lowBlock->RemoveInstruction(curr);

					lowModule->removedSpilledRegisters++;
				}
			}
		}

		curr = currPrevSibling;
	}
}

void OptimizeFirstImmediateTemporaryUse(VmLoweredModule *lowModule, VmLoweredBlock *lowBlock)
{
	// Go from last instruction back to first
	for(VmLoweredInstruction *curr = lowBlock->lastInstruction; curr;)
	{
		// We are looking for register spill loads
		if(!IsSpillLoad(curr))
		{
			curr = curr->prevSibling;
			continue;
		}

		// Spill variable
		VariableData *spillVariable = curr->argument->container;

		// For safety, check that all register spill users are from the current block
		if(HasOtherBlockUse(spillVariable, lowBlock))
		{
			curr = curr->prevSibling;
			continue;
		}

		VmLoweredInstruction *currPrevSibling = curr->prevSibling;

		// Try to peephole first spill read if it comes directly after store
		if(spillVariable->lowUsers.size() > 2)
		{
			assert(IsSpillStore(spillVariable->lowUsers[0]));

			VmLoweredInstruction *targetStore = spillVariable->lowUsers[0];

			if(curr->prevSibling && curr->prevSibling->prevSibling == targetStore)
			{
				// Check that target store data is not used after it
				if(targetStore->nextSibling->cmd == cmdPop && targetStore->nextSibling->argument->iValue == targetStore->helper->iValue)
				{
					VmLoweredInstruction *originalPop = targetStore->nextSibling;

					assert(curr->helper->iValue == targetStore->helper->iValue);

					// Detach original instructions
					lowBlock->RemoveInstruction(originalPop);

					currPrevSibling = curr->prevSibling;

					lowBlock->RemoveInstruction(curr);

					lowModule->removedSpilledRegisters++;
				}
			}
		}

		curr = currPrevSibling;
	}
}

bool HasWritesToVariables(ArrayView<VariableData*> writeList, ArrayView<VariableData*> accessList)
{
	for(unsigned i = 0; i < writeList.size(); i++)
	{
		VariableData *write = writeList[i];

		for(unsigned k = 0; k < accessList.size(); k++)
		{
			if(write == accessList[k])
				return true;
		}
	}

	return false;
}

void OptimizeTemporaryCreationPlacement(VmLoweredModule *lowModule, VmLoweredBlock *lowBlock)
{
	for(VmLoweredInstruction *curr = lowBlock->firstInstruction; curr;)
	{
		VmLoweredInstruction *next = curr->nextSibling;

		if(!IsSpillStore(curr))
		{
			curr = next;
			continue;
		}

		// Spill variable
		VariableData *spillVariable = curr->argument->container;

		// For safety, check that all register spill users are from the current block
		if(HasOtherBlockUse(spillVariable, lowBlock))
		{
			curr = next;
			continue;
		}

		if(spillVariable->lowUsers.size() >= 2)
		{
			VmLoweredInstruction *targetStore = spillVariable->lowUsers[0];

			assert(curr == targetStore);

			// Check that target store data is not used after it
			if(targetStore->nextSibling->cmd == cmdPop && targetStore->nextSibling->argument->iValue == targetStore->helper->iValue)
			{
				VmLoweredInstruction *originalPop = targetStore->nextSibling;

				VmLoweredInstruction *blockEnd = targetStore->nextSibling;

				unsigned stackDepth = blockEnd->stackDepthAfter;

				// Find beginning of the block that provides value for our store
				VmLoweredInstruction *blockStart = targetStore;

				while(blockStart)
				{
					if(blockStart->stackDepthBefore == stackDepth)
						break;

					blockStart = blockStart->prevSibling;
				}

				assert(blockStart);

				// Find what kind of operations are made in the block
				SmallArray<VariableData*, 16> blockAllocaWrites(lowModule->allocator);
				bool blockHasUnknownMemoryWrite = false;

				SmallArray<VariableData*, 16> blockAllocaReads(lowModule->allocator);
				bool blockHasUnknownMemoryAccess = false;

				for(VmLoweredInstruction *inst = blockStart; inst != blockEnd; inst = inst->nextSibling)
				{
					if(inst == targetStore)
						continue;

					if(VariableData *container = GetMemoryWriteOfAlloca(inst))
						blockAllocaWrites.push_back(container);
					else if(HasMemoryWrite(inst))
						blockHasUnknownMemoryWrite = true;
					else if(VariableData *container = GetMemoryReadOfAlloca(inst))
						blockAllocaReads.push_back(container);
					else if(HasMemoryAccess(inst))
						blockHasUnknownMemoryAccess = true;
				}

				// Find what kind of operations are made after the block before the first load
				VmLoweredInstruction *firstLoad = NULL;
				
				for(VmLoweredInstruction *inst = targetStore->nextSibling; inst; inst = inst->nextSibling)
				{
					for(unsigned i = 1; i < spillVariable->lowUsers.size(); i++)
					{
						if(inst == spillVariable->lowUsers[i])
						{
							firstLoad = inst;
							break;
						}
					}

					if(firstLoad)
						break;
				}

				assert(firstLoad);

				SmallArray<VariableData*, 16> followingAllocaWrites(lowModule->allocator);
				bool followingUnknownMemoryWrite = false;

				SmallArray<VariableData*, 16> followingAllocaReads(lowModule->allocator);
				bool followingUnknownMemoryAccess = false;

				for(VmLoweredInstruction *inst = blockEnd->nextSibling; inst != firstLoad; inst = inst->nextSibling)
				{
					if(VariableData *container = GetMemoryWriteOfAlloca(inst))
						followingAllocaWrites.push_back(container);
					else if(HasMemoryWrite(inst))
						followingUnknownMemoryWrite = true;
					else if(VariableData *container = GetMemoryReadOfAlloca(inst))
						followingAllocaReads.push_back(container);
					else if(HasMemoryAccess(inst))
						followingUnknownMemoryAccess = true;
				}

				// Can't reorder memory modifications
				if(blockHasUnknownMemoryWrite && followingUnknownMemoryWrite)
				{
					curr = next;
					continue;
				}

				// Following instructions might read something that block instructions write
				if(blockHasUnknownMemoryWrite && followingUnknownMemoryAccess)
				{
					curr = next;
					continue;
				}

				// Block instructions might read something that following instructions write
				if(blockHasUnknownMemoryAccess && followingUnknownMemoryWrite)
				{
					curr = next;
					continue;
				}

				// Block writes to alloca variables that following instructions also write
				if(HasWritesToVariables(blockAllocaWrites, followingAllocaWrites))
				{
					curr = next;
					continue;
				}

				// Block writes to alloca variables that following instructions read
				if(HasWritesToVariables(blockAllocaWrites, followingAllocaReads))
				{
					curr = next;
					continue;
				}

				// Following instructions write to alloca variables that block instructions also write
				if(HasWritesToVariables(followingAllocaWrites, blockAllocaWrites))
				{
					curr = next;
					continue;
				}

				// Following instructions write to alloca variables that block instructions read
				if(HasWritesToVariables(followingAllocaWrites, blockAllocaReads))
				{
					curr = next;
					continue;
				}

				// Detach whole block of instructions
				if(blockStart->prevSibling)
					blockStart->prevSibling->nextSibling = blockEnd->nextSibling;

				if(blockStart == blockStart->parent->firstInstruction)
					blockStart->parent->firstInstruction = blockEnd->nextSibling;

				if(blockEnd->nextSibling)
					blockEnd->nextSibling->prevSibling = blockStart->prevSibling;

				if(blockEnd == blockEnd->parent->lastInstruction)
					blockEnd->parent->lastInstruction = blockStart->prevSibling;

				next = blockEnd->nextSibling;

				blockStart->prevSibling = NULL;
				blockEnd->nextSibling = NULL;

				// Update instructin stack depth
				unsigned originalDepth = blockStart->stackDepthBefore;
				unsigned newDepth = firstLoad->prevSibling ? firstLoad->prevSibling->stackDepthAfter : 0;

				for(VmLoweredInstruction *inst = blockStart; inst; inst = inst->nextSibling)
				{
					inst->stackDepthBefore -= originalDepth;
					inst->stackDepthAfter -= originalDepth;

					inst->stackDepthBefore += newDepth;
					inst->stackDepthAfter += newDepth;
				}

				// Re-attach the block at new location
				blockEnd->nextSibling = firstLoad;

				blockStart->prevSibling = firstLoad->prevSibling;

				if(firstLoad->prevSibling)
					firstLoad->prevSibling->nextSibling = blockStart;

				if(firstLoad == blockStart->parent->firstInstruction)
					blockStart->parent->firstInstruction = blockStart;

				firstLoad->prevSibling = blockEnd;

				if(spillVariable->lowUsers.size() > 2)
				{
					lowBlock->RemoveInstruction(originalPop);
					lowBlock->RemoveInstruction(firstLoad);
				}
				else
				{
					lowBlock->RemoveInstruction(targetStore);
					lowBlock->RemoveInstruction(originalPop);

					if(next == firstLoad)
						next = firstLoad->nextSibling;

					lowBlock->RemoveInstruction(firstLoad);
				}

				lowModule->removedSpilledRegisters++;
			}
		}

		curr = next;
	}
}

void OptimizeByReadingCopyInsteadOfTemporary(VmLoweredModule *lowModule, VmLoweredBlock *lowBlock)
{
	(void)lowModule;

	// Go from last instruction back to first
	for(VmLoweredInstruction *curr = lowBlock->lastInstruction; curr;)
	{
		// We are looking for register spill loads
		if(!IsSpillLoad(curr))
		{
			curr = curr->prevSibling;
			continue;
		}

		// Spill variable
		VariableData *spillVariable = curr->argument->container;

		// For safety, check that all register spill users are from the current block
		if(HasOtherBlockUse(spillVariable, lowBlock))
		{
			curr = curr->prevSibling;
			continue;
		}

		VmLoweredInstruction *currPrevSibling = curr->prevSibling;

		if(spillVariable->lowUsers.size() >= 2)
		{
			assert(IsSpillStore(spillVariable->lowUsers[0]));

			VmLoweredInstruction *targetStore = spillVariable->lowUsers[0];

			// Check if the next instruction is a store to a different variable
			if(IsVariableStoreOfSize(targetStore->nextSibling, targetStore->helper->iValue))
			{
				VmLoweredInstruction *copyStore = targetStore->nextSibling;

				VariableData *copyVariable = copyStore->argument->container;

				// Look if it's safe to change spill read to the copy read
				VmLoweredInstruction *it = curr->prevSibling;

				bool safeToMove = true;

				while(it != copyStore)
				{
					// Depending on original variable usage type, we might ignore more side-effecting operations
					if(HasAddressTaken(copyVariable))
					{
						if(HasMemoryWriteToVariable(it, copyVariable))
						{
							safeToMove = false;
							break;
						}

						if(HasUnknownMemoryWrite(it))
						{
							safeToMove = false;
							break;
						}
					}
					else
					{
						if(HasMemoryWriteToVariable(it, copyVariable))
						{
							safeToMove = false;
							break;
						}
					}

					it = it->prevSibling;
				}

				if(safeToMove)
				{
					assert(curr->helper->iValue == copyStore->helper->iValue);

					RemoveContainerUse(curr->argument->container, curr);

					curr->flag = copyStore->flag;
					curr->argument = copyStore->argument;

					curr->argument->container->lowUsers.push_back(curr);
				}
			}
		}

		curr = currPrevSibling;
	}
}

void OptimizeTemporaryRegisterSpills(VmLoweredModule *lowModule, VmLoweredFunction *lowFunction)
{
	for(unsigned i = 0; i < lowFunction->blocks.size(); i++)
	{
		VmLoweredBlock *lowBlock = lowFunction->blocks[i];

		if(!lowBlock->firstInstruction)
			continue;

		OptimizeByReadingTargetInsteadOfTemporary(lowModule, lowBlock);

		OptimizeSingleImmediateTemporaryUses(lowModule, lowBlock);

		OptimizeFirstImmediateTemporaryUse(lowModule, lowBlock);

		OptimizeUnusedTemporaries(lowModule, lowBlock);

		OptimizeSingleImmediateTemporaryUses(lowModule, lowBlock);

		OptimizeFirstImmediateTemporaryUse(lowModule, lowBlock);

		OptimizeTemporaryCreationPlacement(lowModule, lowBlock);

		OptimizeByReadingCopyInsteadOfTemporary(lowModule, lowBlock);

		OptimizeUnusedTemporaries(lowModule, lowBlock);
	}
}

void OptimizeTemporaryRegisterSpills(VmLoweredModule *lowModule)
{
	TRACE_SCOPE("InstructionTreeVmLower", "OptimizeTemporaryRegisterSpills");

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		VmLoweredFunction *lowFunction = lowModule->functions[i];

		if(lowFunction->blocks.empty())
			continue;

		TRACE_SCOPE("InstructionTreeVmLower", "FunctionPass");

		if(lowFunction->vmFunction->function && lowFunction->vmFunction->function->name && lowFunction->vmFunction->function->name)
			TRACE_LABEL2(lowFunction->vmFunction->function->name->name.begin, lowFunction->vmFunction->function->name->name.end);

		OptimizeTemporaryRegisterSpills(lowModule, lowFunction);
	}
}

void FinalizeRegisterSpills(ExpressionContext &ctx, VmLoweredModule *lowModule)
{
	TRACE_SCOPE("InstructionTreeVmLower", "FinalizeRegisterSpills");

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		VmLoweredFunction *lowFunction = lowModule->functions[i];

		lowModule->vmModule->currentFunction = lowFunction->vmFunction;

		for(unsigned i = 0; i < lowFunction->vmFunction->allocas.size(); i++)
		{
			VariableData *variable = lowFunction->vmFunction->allocas[i];

			if(!variable->isVmRegSpill)
				continue;

			if(variable->lowUsers.empty())
				continue;

			FinalizeAlloca(ctx, lowModule->vmModule, variable);
		}

		lowModule->vmModule->currentFunction = NULL;
	}
}

void FinalizeInstruction(InstructionVmFinalizeContext &ctx, VmLoweredInstruction *lowInstruction)
{
	ctx.locations.push_back(lowInstruction->location);

	VMCmd cmd;

	cmd.cmd = (CmdID)lowInstruction->cmd;

	if(VmConstant *flag = lowInstruction->flag)
		cmd.flag = (unsigned char)flag->iValue;

	if(VmConstant *helper = lowInstruction->helper)
		cmd.helper = (unsigned short)helper->iValue;

	if(VmConstant *argument = lowInstruction->argument)
	{
		if(VariableData *container = argument->container)
		{
			unsigned moduleId = container->importModule ? container->importModule->importIndex << 24 : 0;

			cmd.argument = argument->iValue + container->offset + moduleId;
		}
		else if(VmFunction *function = argument->fValue)
		{
			if(cmd.cmd == cmdCall || cmd.cmd == cmdFuncAddr)
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
			ctx.fixupPoints.push_back(InstructionVmFinalizeContext::FixupPoint(ctx.cmds.size(), argument->bValue));
			cmd.argument = ~0u;
		}
		else
		{
			cmd.argument = argument->iValue;
		}
	}

	ctx.cmds.push_back(cmd);
}

void FinalizeBlock(InstructionVmFinalizeContext &ctx, VmLoweredBlock *lowBlock)
{
	lowBlock->vmBlock->address = ctx.cmds.size();

	for(VmLoweredInstruction *curr = lowBlock->firstInstruction; curr; curr = curr->nextSibling)
	{
		FinalizeInstruction(ctx, curr);
	}
}

void FinalizeFunction(InstructionVmFinalizeContext &ctx, VmLoweredFunction *lowFunction)
{
	lowFunction->vmFunction->vmAddress = ctx.cmds.size();

	if(FunctionData *data = lowFunction->vmFunction->function)
	{
		assert(data->argumentsSize < 65536);

		// Stack frame should remain aligned, so its size should multiple of 16
		unsigned size = (data->stackSize + 0xf) & ~0xf;

		// Save previous stack frame, and expand current by shift bytes
		ctx.locations.push_back(data->source);
		ctx.cmds.push_back(VMCmd(cmdPushVTop, (unsigned short)data->argumentsSize, size));
	}

	for(unsigned i = 0; i < lowFunction->blocks.size(); i++)
	{
		VmLoweredBlock *lowBlock = lowFunction->blocks[i];

		FinalizeBlock(ctx, lowBlock);
	}

	for(unsigned i = 0; i < ctx.fixupPoints.size(); i++)
	{
		InstructionVmFinalizeContext::FixupPoint &point = ctx.fixupPoints[i];

		assert(point.target);
		assert(point.target->address != ~0u);

		ctx.cmds[point.cmdIndex].argument = point.target->address;
	}

	ctx.fixupPoints.clear();

	lowFunction->vmFunction->vmCodeSize = ctx.cmds.size() - lowFunction->vmFunction->vmAddress;

	ctx.currentFunction = NULL;
}

void FinalizeModule(InstructionVmFinalizeContext &ctx, VmLoweredModule *lowModule)
{
	TRACE_SCOPE("InstructionTreeVmLower", "FinalizeModule");

	ctx.locations.push_back(NULL);
	ctx.cmds.push_back(VMCmd(cmdJmp, 0));

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		VmLoweredFunction *lowFunction = lowModule->functions[i];

		if(!lowFunction->vmFunction->function)
		{
			lowModule->vmModule->vmGlobalCodeStart = ctx.cmds.size();

			ctx.cmds[0].argument = lowModule->vmModule->vmGlobalCodeStart;
		}

		FinalizeFunction(ctx, lowFunction);
	}
}
