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
		if(FunctionData *function = ctx.functions[instruction->argument->iValue])
		{
			stackDepth -= int(function->argumentsSize) / 4;
			stackDepth += int(function->type->returnType->size) / 4;
		}
		else
		{
			assert(!"filed to find function");
		}
		break;
	case cmdCallPtr:
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
	case cmdCreateClosure:
		stackDepth -= pointerSlots;
		break;
	case cmdCloseUpvals:
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
			if(container->lowUsers.back() != instruction)
				memmove(&container->lowUsers[i], &container->lowUsers[i + 1], sizeof(container->lowUsers[0]) * (container->lowUsers.size() - i - 1));
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
		if(!HasAddressTaken(lowInstruction->argument->container))
			return false;

		return true;
	case cmdMovCharStk:
	case cmdMovShortStk:
	case cmdMovIntStk:
	case cmdMovFloatStk:
	case cmdMovDorLStk:
	case cmdMovCmplxStk:
	case cmdSetRangeStk:
	case cmdCall:
	case cmdCallPtr:
	case cmdCreateClosure:
	case cmdCloseUpvals:
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
		if(!HasAddressTaken(lowInstruction->argument->container))
			return false;

		return true;
	case cmdPushCharStk:
	case cmdPushShortStk:
	case cmdPushIntStk:
	case cmdPushFloatStk:
	case cmdPushDorLStk:
	case cmdPushCmplxStk:
		return true;
	case cmdMovChar:
	case cmdMovShort:
	case cmdMovInt:
	case cmdMovFloat:
	case cmdMovDorL:
	case cmdMovCmplx:
		if(!HasAddressTaken(lowInstruction->argument->container))
			return false;

		return true;
	case cmdMovCharStk:
	case cmdMovShortStk:
	case cmdMovIntStk:
	case cmdMovFloatStk:
	case cmdMovDorLStk:
	case cmdMovCmplxStk:
	case cmdSetRangeStk:
	case cmdCall:
	case cmdCallPtr:
	case cmdCreateClosure:
	case cmdCloseUpvals:
		return true;
	default:
		break;
	}

	return false;
}

void LowerIntoBlock(ExpressionContext &ctx, VmLoweredBlock *lowBlock, VmValue *value)
{
	if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		switch(inst->cmd)
		{
		case VM_INST_LOAD_BYTE:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushChar, IsLocalScope(constant->container->scope), 1, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdPushCharStk, 1, offset->iValue);
			}
			break;
		case VM_INST_LOAD_SHORT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushShort, IsLocalScope(constant->container->scope), 2, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdPushShortStk, 2, offset->iValue);
			}
			break;
		case VM_INST_LOAD_INT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushInt, IsLocalScope(constant->container->scope), 4, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdPushIntStk, 4, offset->iValue);
			}
			break;
		case VM_INST_LOAD_FLOAT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushFloat, IsLocalScope(constant->container->scope), 4, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdPushFloatStk, 4, offset->iValue);
			}
			break;
		case VM_INST_LOAD_DOUBLE:
		case VM_INST_LOAD_LONG:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, cmdPushDorL, IsLocalScope(constant->container->scope), 8, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, cmdPushDorLStk, 8, offset->iValue);
			}
			break;
		case VM_INST_LOAD_STRUCT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, inst->type.size == 8 ? cmdPushDorL : cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->type.size, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
				lowBlock->AddInstruction(ctx, inst->source, inst->type.size == 8 ? cmdPushDorLStk : cmdPushCmplxStk, (unsigned short)inst->type.size, offset->iValue);
			}
			break;
		case VM_INST_LOAD_IMMEDIATE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			break;
		case VM_INST_STORE_BYTE:
			LowerIntoBlock(ctx, lowBlock, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

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
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

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
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

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
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

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
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

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
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				(void)offset;
				assert(offset->iValue == 0);

				lowBlock->AddInstruction(ctx, inst->source, inst->arguments[2]->type.size == 8 ? cmdMovDorL : cmdMovCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->arguments[2]->type.size, constant);
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
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

			lowBlock->AddInstruction(ctx, inst->source, cmdIndexStk, (unsigned short)elementSize->iValue, 0);
		}
		break;
		case VM_INST_FUNCTION_ADDRESS:
		{
			VmConstant *funcIndex = getType<VmConstant>(inst->arguments[0]);

			assert(funcIndex);

			lowBlock->AddInstruction(ctx, inst->source, cmdFuncAddr, funcIndex->iValue);
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
			assert(!"not implemented");
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
			VmInstruction *target = getType<VmInstruction>(inst->arguments[0]);

			assert(target);

			unsigned short helper = (unsigned short)inst->type.size;

			// Special cases for simple types
			if(inst->type == VmType::Int)
				helper = bitRetSimple | OTYPE_INT;
			else if(inst->type == VmType::Double)
				helper = bitRetSimple | OTYPE_DOUBLE;
			else if(inst->type == VmType::Long)
				helper = bitRetSimple | OTYPE_LONG;

			if(target->cmd == VM_INST_CONSTRUCT)
			{
				VmValue *context = target->arguments[0];
				VmFunction *function = getType<VmFunction>(target->arguments[1]);

				LowerIntoBlock(ctx, lowBlock, context);

				for(int i = int(inst->arguments.size() - 1); i >= 1; i--)
				{
					LowerIntoBlock(ctx, lowBlock, inst->arguments[i]);

					if(inst->arguments[i]->type.size == 0)
						lowBlock->AddInstruction(ctx, inst->arguments[i]->source, cmdPushImmt, 0u);
				}

				lowBlock->AddInstruction(ctx, inst->source, cmdCall, helper, function->function->functionIndex);
			}
			else
			{
				unsigned paramSize = NULLC_PTR_SIZE;

				LowerIntoBlock(ctx, lowBlock, target);

				for(int i = int(inst->arguments.size() - 1); i >= 1; i--)
				{
					LowerIntoBlock(ctx, lowBlock, inst->arguments[i]);

					if(inst->arguments[i]->type.size == 0)
						lowBlock->AddInstruction(ctx, inst->arguments[i]->source, cmdPushImmt, 0u);

					unsigned size = inst->arguments[i]->type.size;

					paramSize += size > 4 ? size : 4;
				}

				lowBlock->AddInstruction(ctx, inst->source, cmdCallPtr, helper, paramSize);
			}
		}
		break;
		case VM_INST_RETURN:
		{
			bool localReturn = lowBlock->vmBlock->parent->function != NULL;

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
					LowerIntoBlock(ctx, lowBlock, inst->arguments[i]);
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
			LowerIntoBlock(ctx, lowBlock, inst->arguments[0]);
			break;
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

void FinalizeRegisterSpills(ExpressionContext &ctx, VmLoweredModule *lowModule)
{
	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		VmLoweredFunction *lowFunction = lowModule->functions[i];

		lowModule->vmModule->currentFunction = lowFunction->vmFunction;

		for(unsigned i = 0; i < lowFunction->vmFunction->allocas.size(); i++)
		{
			VariableData *variable = lowFunction->vmFunction->allocas[i];

			if(!variable->isVmRegSpill)
				continue;

			if(variable->users.empty())
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
			if(cmd.cmd == cmdPushVTop)
			{
				FunctionData *data = function->function;

				assert(data->argumentsSize < 65536);

				// Stack frame should remain aligned, so its size should multiple of 16
				unsigned size = (data->stackSize + 0xf) & ~0xf;

				// Save previous stack frame, and expand current by shift bytes
				cmd.helper = (unsigned short)data->argumentsSize;
				cmd.argument = size;
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
	lowFunction->vmFunction->address = ctx.cmds.size();

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

	lowFunction->vmFunction->codeSize = ctx.cmds.size() - lowFunction->vmFunction->address;

	ctx.currentFunction = NULL;
}

void FinalizeModule(InstructionVmFinalizeContext &ctx, VmLoweredModule *lowModule)
{
	ctx.locations.push_back(NULL);
	ctx.cmds.push_back(VMCmd(cmdJmp, 0));

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		VmLoweredFunction *lowFunction = lowModule->functions[i];

		if(!lowFunction->vmFunction->function)
		{
			lowModule->vmModule->globalCodeStart = ctx.cmds.size();

			ctx.cmds[0].argument = lowModule->vmModule->globalCodeStart;
		}

		FinalizeFunction(ctx, lowFunction);
	}
}
