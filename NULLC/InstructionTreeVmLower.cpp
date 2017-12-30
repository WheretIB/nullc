#include "InstructionTreeVmLower.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

typedef InstructionVMLowerContext Context;

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
				ctx.cmds.push_back(VMCmd(cmdPop, curr->type.size));
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
				ctx.cmds.push_back(VMCmd(cmdPushInt, IsLocalScope(constant->container->scope), 1, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdPushIntStk, 1, 0));
			}
			break;
		case VM_INST_LOAD_SHORT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdPushShort, IsLocalScope(constant->container->scope), 2, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdPushShortStk, 2, 0));
			}
			break;
		case VM_INST_LOAD_INT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdPushInt, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdPushIntStk, 4, 0));
			}
			break;
		case VM_INST_LOAD_FLOAT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdPushFloat, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdPushFloatStk, 4, 0));
			}
			break;
		case VM_INST_LOAD_DOUBLE:
		case VM_INST_LOAD_LONG:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdPushDorL, IsLocalScope(constant->container->scope), 8, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdPushDorLStk, 8, 0));
			}
			break;
		case VM_INST_LOAD_STRUCT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->type.size, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdPushCmplxStk, (unsigned short)inst->type.size, 0));
			}
			break;
		case VM_INST_LOAD_IMMEDIATE:
			Lower(ctx, inst->arguments[0]);
			break;
		case VM_INST_STORE_BYTE:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdMovChar, IsLocalScope(constant->container->scope), 1, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdMovCharStk, 1, 0));
			}

			ctx.cmds.push_back(VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_SHORT:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdMovShort, IsLocalScope(constant->container->scope), 2, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdMovShortStk, 2, 0));
			}

			ctx.cmds.push_back(VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_INT:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdMovInt, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdMovIntStk, 4, 0));
			}

			ctx.cmds.push_back(VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_FLOAT:
			Lower(ctx, inst->arguments[1]);

			ctx.cmds.push_back(VMCmd(cmdDtoF));

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdMovFloat, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdMovFloatStk, 4, 0));
			}

			ctx.cmds.push_back(VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_DOUBLE:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdMovDorL, IsLocalScope(constant->container->scope), 8, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdMovDorLStk, 8, 0));
			}

			ctx.cmds.push_back(VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_STORE_STRUCT:
			Lower(ctx, inst->arguments[1]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				ctx.cmds.push_back(VMCmd(cmdMovCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->arguments[1]->type.size, constant->iValue + constant->container->offset));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);
				ctx.cmds.push_back(VMCmd(cmdMovCmplxStk, (unsigned short)inst->arguments[1]->type.size, 0));
			}

			ctx.cmds.push_back(VMCmd(cmdPop, inst->arguments[1]->type.size));
			break;
		case VM_INST_DOUBLE_TO_INT:
			ctx.cmds.push_back(VMCmd(cmdDtoI));
			break;
		case VM_INST_DOUBLE_TO_LONG:
			ctx.cmds.push_back(VMCmd(cmdDtoL));
			break;
		case VM_INST_DOUBLE_TO_FLOAT:
			ctx.cmds.push_back(VMCmd(cmdDtoF));
			break;
		case VM_INST_INT_TO_DOUBLE:
			ctx.cmds.push_back(VMCmd(cmdItoD));
			break;
		case VM_INST_LONG_TO_DOUBLE:
			ctx.cmds.push_back(VMCmd(cmdLtoD));
			break;
		case VM_INST_INT_TO_LONG:
			ctx.cmds.push_back(VMCmd(cmdItoL));
			break;
		case VM_INST_LONG_TO_INT:
			ctx.cmds.push_back(VMCmd(cmdLtoI));
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

				ctx.cmds.push_back(VMCmd(cmdIndex, (unsigned short)elementSize->iValue, arrSize->iValue));
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

				ctx.cmds.push_back(VMCmd(cmdIndexStk, (unsigned short)elementSize->iValue, 0));
			}
			break;
		case VM_INST_FUNCTION_ADDRESS:
			{
				VmConstant *funcIndex = getType<VmConstant>(inst->arguments[0]);

				assert(funcIndex);

				ctx.cmds.push_back(VMCmd(cmdFuncAddr, funcIndex->iValue));
			}
			break;
		case VM_INST_TYPE_ID:
			{
				VmConstant *typeIndex = getType<VmConstant>(inst->arguments[0]);

				assert(typeIndex);

				ctx.cmds.push_back(VMCmd(cmdPushTypeID, typeIndex->iValue));
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
				ctx.cmds.push_back(VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_JUMP_Z:
			assert(inst->arguments[0]->type.size == 4);

			Lower(ctx, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1])
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				ctx.cmds.push_back(VMCmd(cmdJmpNZ, ~0u));
			}
			else
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				ctx.cmds.push_back(VMCmd(cmdJmpZ, ~0u));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				ctx.cmds.push_back(VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_JUMP_NZ:
			assert(inst->arguments[0]->type.size == 4);

			Lower(ctx, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1])
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				ctx.cmds.push_back(VMCmd(cmdJmpZ, ~0u));
			}
			else
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				ctx.cmds.push_back(VMCmd(cmdJmpNZ, ~0u));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				ctx.cmds.push_back(VMCmd(cmdJmp, ~0u));
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

					ctx.cmds.push_back(VMCmd(cmdCall, helper, function->function->functionIndex));
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

					ctx.cmds.push_back(VMCmd(cmdCallPtr, helper, paramSize));
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

					ctx.cmds.push_back(VMCmd(cmdReturn, operType, (unsigned short)localReturn, result->type.size));
				}
				else
				{
					ctx.cmds.push_back(VMCmd(cmdReturn, OTYPE_COMPLEX, (unsigned short)localReturn, 0));
				}
			}
			break;
		case VM_INST_YIELD:
			ctx.cmds.push_back(VMCmd(cmdNop));
			break;
		case VM_INST_ADD:
			{
				bool isContantOneLhs = DoesConstantMatchEither(inst->arguments[0], 1, 1.0f, 1ll);

				if(isContantOneLhs || DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
				{
					Lower(ctx, isContantOneLhs ? inst->arguments[1] : inst->arguments[0]);

					if(inst->type == VmType::Int)
						ctx.cmds.push_back(VMCmd(cmdIncI));
					else if(inst->type == VmType::Double)
						ctx.cmds.push_back(VMCmd(cmdIncD));
					else if(inst->type == VmType::Long)
						ctx.cmds.push_back(VMCmd(cmdIncL));
				}
				else
				{
					Lower(ctx, inst->arguments[0]);
					Lower(ctx, inst->arguments[1]);

					if(inst->type == VmType::Int)
						ctx.cmds.push_back(VMCmd(cmdAdd));
					else if(inst->type == VmType::Double)
						ctx.cmds.push_back(VMCmd(cmdAddD));
					else if(inst->type == VmType::Long)
						ctx.cmds.push_back(VMCmd(cmdAddL));
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
						ctx.cmds.push_back(VMCmd(cmdDecI));
					else if(inst->type == VmType::Double)
						ctx.cmds.push_back(VMCmd(cmdDecD));
					else if(inst->type == VmType::Long)
						ctx.cmds.push_back(VMCmd(cmdDecL));
				}
				else
				{
					Lower(ctx, inst->arguments[0]);
					Lower(ctx, inst->arguments[1]);

					if(inst->type == VmType::Int)
						ctx.cmds.push_back(VMCmd(cmdSub));
					else if(inst->type == VmType::Double)
						ctx.cmds.push_back(VMCmd(cmdSubD));
					else if(inst->type == VmType::Long)
						ctx.cmds.push_back(VMCmd(cmdSubL));
				}
			}
			break;
		case VM_INST_MUL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdMul));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdMulD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdMulL));
			break;
		case VM_INST_DIV:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdDiv));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdDivD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdDivL));
			break;
		case VM_INST_POW:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdPow));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdPowD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdPowL));
			break;
		case VM_INST_MOD:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdMod));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdModD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdModL));
			break;
		case VM_INST_LESS:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdLess));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdLessD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdLessL));
			break;
		case VM_INST_GREATER:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdGreater));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdGreaterD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdGreaterL));
			break;
		case VM_INST_LESS_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdLEqual));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdLEqualD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdLEqualL));
			break;
		case VM_INST_GREATER_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdGEqual));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdGEqualD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdGEqualL));
			break;
		case VM_INST_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdEqual));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdEqualD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdEqualL));
			break;
		case VM_INST_NOT_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdNEqual));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdNEqualD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdNEqualL));
			break;
		case VM_INST_SHL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdShl));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdShlL));
			break;
		case VM_INST_SHR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdShr));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdShrL));
			break;
		case VM_INST_BIT_AND:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdBitAnd));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdBitAndL));
			break;
		case VM_INST_BIT_OR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdBitOr));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdBitOrL));
			break;
		case VM_INST_BIT_XOR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdBitXor));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdBitXorL));
			break;
		case VM_INST_LOG_XOR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdLogXor));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdLogXorL));
			break;
		case VM_INST_NEG:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdNeg));
			else if(inst->type == VmType::Double)
				ctx.cmds.push_back(VMCmd(cmdNegD));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdNegL));
			break;
		case VM_INST_BIT_NOT:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdBitNot));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdBitNotL));
			break;
		case VM_INST_LOG_NOT:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				ctx.cmds.push_back(VMCmd(cmdLogNot));
			else if(inst->type == VmType::Long)
				ctx.cmds.push_back(VMCmd(cmdLogNotL));
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

				ctx.cmds.push_back(VMCmd(cmdConvertPtr, typeIndex->iValue));
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
					ctx.cmds.push_back(VMCmd(cmdFuncAddr, function->function->functionIndex));
				else
					Lower(ctx, inst->arguments[i]);
			}
			break;
		case VM_INST_EXTRACT:
			ctx.cmds.push_back(VMCmd(cmdNop));
			break;
		case VM_INST_UNYIELD:
			ctx.cmds.push_back(VMCmd(cmdNop));
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
			ctx.cmds.push_back(VMCmd(cmdPushImmt, constant->iValue));
		}
		else if(constant->type == VmType::Double)
		{
			unsigned data[2];
			memcpy(data, &constant->dValue, 8);

			ctx.cmds.push_back(VMCmd(cmdPushImmt, data[1]));
			ctx.cmds.push_back(VMCmd(cmdPushImmt, data[0]));
		}
		else if(constant->type == VmType::Long)
		{
			unsigned data[2];
			memcpy(data, &constant->lValue, 8);

			ctx.cmds.push_back(VMCmd(cmdPushImmt, data[1]));
			ctx.cmds.push_back(VMCmd(cmdPushImmt, data[0]));
		}
		else if(constant->type.type == VM_TYPE_POINTER)
		{
			if(!constant->container)
			{
				assert(constant->iValue == 0);

				ctx.cmds.push_back(VMCmd(cmdPushPtrImmt, 0));
			}
			else
			{
				ctx.cmds.push_back(VMCmd(cmdGetAddr, IsLocalScope(constant->container->scope), constant->iValue + constant->container->offset));
			}
		}
		else if(constant->type.type == VM_TYPE_STRUCT)
		{
			assert(constant->type.size % 4 == 0);

			for(unsigned i = 0; i < constant->type.size / 4; i++)
				ctx.cmds.push_back(VMCmd(cmdPushImmt, ((unsigned*)constant->sValue)[i]));
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
	// TODO: Source code locations

	for(unsigned i = 0; i < ctx.cmds.size(); i++)
	{
		VMCmd &cmd = ctx.cmds[i];

		char buf[256];
		cmd.Decode(buf);

		fprintf(ctx.file, "// %s\n", buf);
	}
}
