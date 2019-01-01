#include "InstructionTreeVmLower.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

typedef InstructionVMLowerContext Context;

void AddCommand(Context &ctx, SynBase *source, VMCmd cmd)
{
	ctx.locations.push_back(source);
	ctx.cmds.push_back(cmd);
}

void Lower(Context &ctx, VmValue *value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		assert(ctx.currentFunction == NULL);

		ctx.currentFunction = function;

		function->address = ctx.cmds.size();

		for(VmBlock *curr = function->firstBlock; curr; curr = curr->nextSibling)
			Lower(ctx, curr);

		for(unsigned i = 0; i < ctx.fixupPoints.size(); i++)
		{
			Context::FixupPoint &point = ctx.fixupPoints[i];

			assert(point.target);
			assert(point.target->address != ~0u);

			ctx.cmds[point.cmdIndex].argument = point.target->address;
		}

		ctx.fixupPoints.clear();

		ctx.currentFunction = NULL;
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		assert(ctx.currentBlock == NULL);

		ctx.currentBlock = block;

		block->address = ctx.cmds.size();

		for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
		{
			if(!curr->users.empty())
				continue;

			Lower(ctx, curr);

			if(curr->type.size)
				AddCommand(ctx, block->source, VMCmd(cmdPop, curr->type.size));
		}

		ctx.currentBlock = NULL;
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		switch(inst->cmd)
		{
		case VM_INST_LOAD_BYTE:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdPushInt, IsLocalScope(constant->container->scope), 1, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushIntStk, 1, 0));
			}
			break;
		case VM_INST_LOAD_SHORT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdPushShort, IsLocalScope(constant->container->scope), 2, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushShortStk, 2, 0));
			}
			break;
		case VM_INST_LOAD_INT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdPushInt, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushIntStk, 4, 0));
			}
			break;
		case VM_INST_LOAD_FLOAT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdPushFloat, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushFloatStk, 4, 0));
			}
			break;
		case VM_INST_LOAD_DOUBLE:
		case VM_INST_LOAD_LONG:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdPushDorL, IsLocalScope(constant->container->scope), 8, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushDorLStk, 8, 0));
			}
			break;
		case VM_INST_LOAD_STRUCT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->type.size, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushCmplxStk, (unsigned short)inst->type.size, 0));
			}
			break;
		case VM_INST_LOAD_IMMEDIATE:
			Lower(ctx, inst->arguments[0]);
			break;
		case VM_INST_STORE_BYTE:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdMovChar, IsLocalScope(constant->container->scope), 1, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovCharStk, 1, 0));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_SHORT:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdMovShort, IsLocalScope(constant->container->scope), 2, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovShortStk, 2, 0));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_INT:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdMovInt, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovIntStk, 4, 0));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_FLOAT:
			Lower(ctx, inst->arguments[1]);

			AddCommand(ctx, inst->source, VMCmd(cmdDtoF));

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdMovFloat, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovFloatStk, 4, 0));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_DOUBLE:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdMovDorL, IsLocalScope(constant->container->scope), 8, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovDorLStk, 8, 0));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_STRUCT:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				AddCommand(ctx, inst->source, VMCmd(cmdMovCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->arguments[1]->type.size, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovCmplxStk, (unsigned short)inst->arguments[1]->type.size, 0));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_DOUBLE_TO_INT:
			AddCommand(ctx, inst->source, VMCmd(cmdDtoI));
			break;
		case VM_INST_DOUBLE_TO_LONG:
			AddCommand(ctx, inst->source, VMCmd(cmdDtoL));
			break;
		case VM_INST_DOUBLE_TO_FLOAT:
			AddCommand(ctx, inst->source, VMCmd(cmdDtoF));
			break;
		case VM_INST_INT_TO_DOUBLE:
			AddCommand(ctx, inst->source, VMCmd(cmdItoD));
			break;
		case VM_INST_LONG_TO_DOUBLE:
			AddCommand(ctx, inst->source, VMCmd(cmdLtoD));
			break;
		case VM_INST_INT_TO_LONG:
			AddCommand(ctx, inst->source, VMCmd(cmdItoL));
			break;
		case VM_INST_LONG_TO_INT:
			AddCommand(ctx, inst->source, VMCmd(cmdLtoI));
			break;
		case VM_INST_INDEX:
			{
				VmConstant *arrSize = getType<VmConstant>(inst->arguments[0]);
				VmConstant *elementSize = getType<VmConstant>(inst->arguments[1]);
				VmValue *pointer = inst->arguments[2];
				VmValue *index = inst->arguments[3];

				assert(arrSize && elementSize);

				Lower(ctx, pointer);
				Lower(ctx, index);

				AddCommand(ctx, inst->source, VMCmd(cmdIndex, (unsigned short)elementSize->iValue, arrSize->iValue));
			}
			break;
		case VM_INST_INDEX_UNSIZED:
			{
				VmConstant *elementSize = getType<VmConstant>(inst->arguments[0]);
				VmValue *arr = inst->arguments[1];
				VmValue *index = inst->arguments[2];

				assert(elementSize);

				Lower(ctx, arr);
				Lower(ctx, index);

				AddCommand(ctx, inst->source, VMCmd(cmdIndexStk, (unsigned short)elementSize->iValue, 0));
			}
			break;
		case VM_INST_FUNCTION_ADDRESS:
			{
				VmConstant *funcIndex = getType<VmConstant>(inst->arguments[0]);

				assert(funcIndex);

				AddCommand(ctx, inst->source, VMCmd(cmdFuncAddr, funcIndex->iValue));
			}
			break;
		case VM_INST_TYPE_ID:
			{
				VmConstant *typeIndex = getType<VmConstant>(inst->arguments[0]);

				assert(typeIndex);

				AddCommand(ctx, inst->source, VMCmd(cmdPushTypeID, typeIndex->iValue));
			}
			break;
		case VM_INST_SET_RANGE:
			assert(!"not implemented");
			break;
		case VM_INST_JUMP:
			// Check if jump is fall-through
			if(!(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[0]))
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[0])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_JUMP_Z:
			assert(inst->arguments[0]->type.size == 4);

			Lower(ctx, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1])
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpNZ, ~0u));
			}
			else
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpZ, ~0u));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_JUMP_NZ:
			assert(inst->arguments[0]->type.size == 4);

			Lower(ctx, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1])
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpZ, ~0u));
			}
			else
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpNZ, ~0u));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
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

					Lower(ctx, context);

					for(unsigned i = 1; i < inst->arguments.size(); i++)
						Lower(ctx, inst->arguments[i]);

					AddCommand(ctx, inst->source, VMCmd(cmdCall, helper, function->function->functionIndex));
				}
				else
				{
					unsigned paramSize = NULLC_PTR_SIZE;

					Lower(ctx, target);

					for(unsigned i = 1; i < inst->arguments.size(); i++)
					{
						Lower(ctx, inst->arguments[i]);

						unsigned size = inst->arguments[i]->type.size;

						paramSize += size > 4 ? size : 4;
					}

					AddCommand(ctx, inst->source, VMCmd(cmdCallPtr, helper, paramSize));
				}
			}
			break;
		case VM_INST_RETURN:
			{
				bool localReturn = ctx.currentFunction->function != NULL;

				if(!inst->arguments.empty())
				{
					VmValue *result = inst->arguments[0];

					Lower(ctx, result);

					unsigned char operType = OTYPE_COMPLEX;

					if(result->type == VmType::Int)
						operType = OTYPE_INT;
					else if(result->type == VmType::Double)
						operType = OTYPE_DOUBLE;
					else if(result->type == VmType::Long)
						operType = OTYPE_LONG;

					AddCommand(ctx, inst->source, VMCmd(cmdReturn, operType, (unsigned short)localReturn, result->type.size));
				}
				else
				{
					AddCommand(ctx, inst->source, VMCmd(cmdReturn, OTYPE_COMPLEX, (unsigned short)localReturn, 0));
				}
			}
			break;
		case VM_INST_YIELD:
			AddCommand(ctx, inst->source, VMCmd(cmdNop));
			break;
		case VM_INST_ADD:
			{
				bool isContantOneLhs = DoesConstantMatchEither(inst->arguments[0], 1, 1.0f, 1ll);

				if(isContantOneLhs || DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
				{
					Lower(ctx, isContantOneLhs ? inst->arguments[1] : inst->arguments[0]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdIncI));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdIncD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdIncL));
				}
				else
				{
					Lower(ctx, inst->arguments[0]);
					Lower(ctx, inst->arguments[1]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdAdd));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdAddD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdAddL));
				}
			}
			break;
		case VM_INST_SUB:
			{
				bool isContantOneLhs = DoesConstantMatchEither(inst->arguments[0], 1, 1.0f, 1ll);

				if(isContantOneLhs || DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
				{
					Lower(ctx, isContantOneLhs ? inst->arguments[1] : inst->arguments[0]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdDecI));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdDecD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdDecL));
				}
				else
				{
					Lower(ctx, inst->arguments[0]);
					Lower(ctx, inst->arguments[1]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdSub));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdSubD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdSubL));
				}
			}
			break;
		case VM_INST_MUL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdMul));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdMulD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdMulL));
			break;
		case VM_INST_DIV:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdDiv));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdDivD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdDivL));
			break;
		case VM_INST_POW:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdPow));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdPowD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdPowL));
			break;
		case VM_INST_MOD:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdMod));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdModD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdModL));
			break;
		case VM_INST_LESS:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLess));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdLessD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLessL));
			break;
		case VM_INST_GREATER:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdGreater));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdGreaterD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdGreaterL));
			break;
		case VM_INST_LESS_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLEqual));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdLEqualD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLEqualL));
			break;
		case VM_INST_GREATER_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdGEqual));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdGEqualD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdGEqualL));
			break;
		case VM_INST_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdEqual));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdEqualD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdEqualL));
			break;
		case VM_INST_NOT_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdNEqual));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdNEqualD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdNEqualL));
			break;
		case VM_INST_SHL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdShl));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdShlL));
			break;
		case VM_INST_SHR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdShr));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdShrL));
			break;
		case VM_INST_BIT_AND:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitAnd));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitAndL));
			break;
		case VM_INST_BIT_OR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitOr));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitOrL));
			break;
		case VM_INST_BIT_XOR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitXor));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitXorL));
			break;
		case VM_INST_LOG_XOR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLogXor));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLogXorL));
			break;
		case VM_INST_NEG:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdNeg));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdNegD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdNegL));
			break;
		case VM_INST_BIT_NOT:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitNot));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitNotL));
			break;
		case VM_INST_LOG_NOT:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLogNot));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLogNotL));
			break;
		case VM_INST_CREATE_CLOSURE:
			assert(!"not implemented");
			break;
		case VM_INST_CLOSE_UPVALUES:
			assert(!"not implemented");
			break;
		case VM_INST_CONVERT_POINTER:
			{
				VmValue *pointer = inst->arguments[0];
				VmConstant *typeIndex = getType<VmConstant>(inst->arguments[1]);

				assert(typeIndex);

				Lower(ctx, pointer);

				AddCommand(ctx, inst->source, VMCmd(cmdConvertPtr, typeIndex->iValue));
			}
			break;
		case VM_INST_CHECKED_RETURN:
			assert(!"not implemented");
			break;
		case VM_INST_CONSTRUCT:
		case VM_INST_ARRAY:
			for(unsigned i = 0; i < inst->arguments.size(); i++)
			{
				VmValue *argument = inst->arguments[i];

				assert(argument->type.size % 4 == 0);
				
				if(VmFunction *function = getType<VmFunction>(argument))
					AddCommand(ctx, inst->source, VMCmd(cmdFuncAddr, function->function->functionIndex));
				else
					Lower(ctx, inst->arguments[i]);
			}
			break;
		case VM_INST_EXTRACT:
			AddCommand(ctx, inst->source, VMCmd(cmdNop));
			break;
		case VM_INST_UNYIELD:
			AddCommand(ctx, inst->source, VMCmd(cmdNop));
			break;
		case VM_INST_BITCAST:
			Lower(ctx, inst->arguments[0]);
			break;
		default:
			assert(!"unknown instruction");
		}
	}
	else if(VmConstant *constant = getType<VmConstant>(value))
	{
		if(constant->type == VmType::Void)
		{
			return;
		}
		else if(constant->type == VmType::Int)
		{
			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, constant->iValue));
		}
		else if(constant->type == VmType::Double)
		{
			unsigned data[2];
			memcpy(data, &constant->dValue, 8);

			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[1]));
			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[0]));
		}
		else if(constant->type == VmType::Long)
		{
			unsigned data[2];
			memcpy(data, &constant->lValue, 8);

			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[1]));
			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[0]));
		}
		else if(constant->type.type == VM_TYPE_POINTER)
		{
			if(!constant->container)
			{
				assert(constant->iValue == 0);

				AddCommand(ctx, constant->source, VMCmd(cmdPushPtrImmt, 0));
			}
			else
			{
				AddCommand(ctx, constant->source, VMCmd(cmdGetAddr, IsLocalScope(constant->container->scope), constant->iValue + constant->container->offset));
			}
		}
		else if(constant->type.type == VM_TYPE_STRUCT)
		{
			assert(constant->type.size % 4 == 0);

			for(unsigned i = 0; i < constant->type.size / 4; i++)
				AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, ((unsigned*)constant->sValue)[i]));
		}
		else
		{
			assert(!"unknown type");
		}
	}
}

void LowerModule(Context &ctx, VmModule *module)
{
	for(VmFunction *value = module->functions.head; value; value = value->next)
	{
		Lower(ctx, value);
	}
}

void PrintInstructions(Context &ctx)
{
	assert(ctx.locations.size() == ctx.cmds.size());

	for(unsigned i = 0; i < ctx.cmds.size(); i++)
	{
		SynBase *source = ctx.locations[i];
		VMCmd &cmd = ctx.cmds[i];

		if(ctx.showSource && source)
		{
			const char *start = source->pos.begin;
			const char *end = start + 1;

			while(start > ctx.ctx.code && *(start - 1) != '\r' && *(start - 1) != '\n')
				start--;

			while(*end && *end != '\r' && *end != '\n')
				end++;

			if (ctx.showAnnotatedSource)
			{
				unsigned startOffset = unsigned(source->pos.begin - start);
				unsigned endOffset = unsigned(source->pos.end - start);

				if(start != ctx.lastStart || startOffset != ctx.lastStartOffset || endOffset != ctx.lastEndOffset)
				{
					fprintf(ctx.file, "%.*s\n", unsigned(end - start), start);

					if (source->pos.end < end)
					{
						for (unsigned i = 0; i < startOffset; i++)
						{
							fprintf(ctx.file, " ");

							if (start[i] == '\t')
								fprintf(ctx.file, "   ");
						}

						for (unsigned i = startOffset; i < endOffset; i++)
						{
							fprintf(ctx.file, "~");

							if (start[i] == '\t')
								fprintf(ctx.file, "~~~");
						}

						fprintf(ctx.file, "\n");
					}

					ctx.lastStart = start;
					ctx.lastStartOffset = startOffset;
					ctx.lastEndOffset = endOffset;
				}
			}
			else
			{
				if(start != ctx.lastStart)
				{
					fprintf(ctx.file, "%.*s\n", unsigned(end - start), start);

					ctx.lastStart = start;
				}
			}
		}

		char buf[256];
		cmd.Decode(buf);

		if(cmd.cmd == cmdCall || cmd.cmd == cmdFuncAddr)
			fprintf(ctx.file, "// %s (%.*s)\n", buf, FMT_ISTR(ctx.ctx.functions[cmd.argument]->name));
		else
			fprintf(ctx.file, "// %s\n", buf);
	}
}
