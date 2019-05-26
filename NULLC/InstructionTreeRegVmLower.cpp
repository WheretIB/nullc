#include "InstructionTreeRegVmLower.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

const char* GetInstructionName(RegVmInstructionCode code)
{
	switch(code)
	{
	case rviNop:
		return "nop";
	case rviLoadByte:
		return "loadb";
	case rviLoadWord:
		return "loadw";
	case rviLoadDword:
		return "load";
	case rviLoadQword:
		return "loadq";
	case rviLoadFloat:
		return "loadf";
	case rviLoadBytePtr:
		return "loadbp";
	case rviLoadWordPtr:
		return "loadwp";
	case rviLoadDwordPtr:
		return "loadp";
	case rviLoadQwordPtr:
		return "loadqp";
	case rviLoadFloatPtr:
		return "loadfp";
	case rviLoadImm:
		return "loadimm";
	case rviLoadImmHigh:
		return "loadimmh";
	case rviStoreByte:
		return "storeb";
	case rviStoreWord:
		return "storew";
	case rviStoreDword:
		return "store";
	case rviStoreQword:
		return "storeq";
	case rviStoreFloat:
		return "storef";
	case rviStoreBytePtr:
		return "storebp";
	case rviStoreWordPtr:
		return "storewp";
	case rviStoreDwordPtr:
		return "storep";
	case rviStoreQwordPtr:
		return "storeqp";
	case rviStoreFloatPtr:
		return "storefp";
	case rviCombinedd:
		return "combdd";
	case rviMov:
		return "mov";
	case rviDtoi:
		return "dtoi";
	case rviDtol:
		return "dtol";
	case rviDtof:
		return "dtof";
	case rviItod:
		return "itod";
	case rviLtod:
		return "ltod";
	case rviItol:
		return "itol";
	case rviLtoi:
		return "ltoi";
	case rviIndex:
		return "index";
	case rviGetAddr:
		return "getaddr";
	case rviSetRange:
		return "setrange";
	case rviJmp:
		return "jmp";
	case rviJmpz:
		return "jmpz";
	case rviJmpnz:
		return "jmpnz";
	case rviPush:
		return "push";
	case rviPushq:
		return "pushq";
	case rviCall:
		return "call";
	case rviCallPtr:
		return "callp";
	case rviReturn:
		return "ret";
	case rviPushvtop:
		return "pushvtop";
	case rviAdd:
		return "add";
	case rviSub:
		return "sub";
	case rviMul:
		return "mul";
	case rviDiv:
		return "div";
	case rviPow:
		return "pow";
	case rviMod:
		return "mod";
	case rviLess:
		return "less";
	case rviGreater:
		return "greater";
	case rviLequal:
		return "lequal";
	case rviGequal:
		return "gequal";
	case rviEqual:
		return "equal";
	case rviNequal:
		return "nequal";
	case rviShl:
		return "shl";
	case rviShr:
		return "shr";
	case rviBitAnd:
		return "bitand";
	case rviBitOr:
		return "bitor";
	case rviBitXor:
		return "bitxor";
	case rviLogXor:
		return "logxor";
	case rviAddl:
		return "addl";
	case rviSubl:
		return "subl";
	case rviMull:
		return "mull";
	case rviDivl:
		return "divl";
	case rviPowl:
		return "powl";
	case rviModl:
		return "modl";
	case rviLessl:
		return "lessl";
	case rviGreaterl:
		return "greaterl";
	case rviLequall:
		return "lequall";
	case rviGequall:
		return "gequall";
	case rviEquall:
		return "equall";
	case rviNequall:
		return "nequall";
	case rviShll:
		return "shll";
	case rviShrl:
		return "shrl";
	case rviBitAndl:
		return "bitandl";
	case rviBitOrl:
		return "bitorl";
	case rviBitXorl:
		return "bitxorl";
	case rviLogXorl:
		return "logxorl";
	case rviAddd:
		return "addd";
	case rviSubd:
		return "subd";
	case rviMuld:
		return "muld";
	case rviDivd:
		return "divd";
	case rviPowd:
		return "powd";
	case rviModd:
		return "modd";
	case rviLessd:
		return "lessd";
	case rviGreaterd:
		return "greaterd";
	case rviLequald:
		return "lequald";
	case rviGequald:
		return "gequald";
	case rviEquald:
		return "equald";
	case rviNequald:
		return "nequald";
	case rviNeg:
		return "neg";
	case rviNegl:
		return "negl";
	case rviNegd:
		return "negd";
	case rviBitNot:
		return "bitnot";
	case rviBitNotl:
		return "bitnotl";
	case rviLogNot:
		return "lognot";
	case rviLogNotl:
		return "lognotl";
	case rviConvertPtr:
		return "convertptr";
	case rviCheckRet:
		return "checkret";
	case rviFuncAddr:
		return "funcaddr";
	case rviTypeid:
		return "typeid";
	default:
		assert(!"unknown instruction");
	}

	return "";
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, RegVmLoweredInstruction* instruction)
{
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
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(location, code, 0, 0, 0, NULL));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(location, code, rA, rB, rC, NULL));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmConstant *argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(location, code, rA, rB, rC, argument));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(location, code, rA, rB, rC, CreateConstantInt(ctx.allocator, NULL, argument)));
}

void RegVmLoweredBlock::AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmBlock *argument)
{
	AddInstruction(ctx, new (ctx.get<RegVmLoweredInstruction>()) RegVmLoweredInstruction(location, code, rA, rB, rC, CreateConstantBlock(ctx.allocator, NULL, argument)));
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
		assert(nextRegister <= 255);

		reg = nextRegister;
		nextRegister++;
	}

	registerUsers[reg]++;

	return reg;
}

void RegVmLoweredFunction::FreeRegister(unsigned char reg)
{
	if(reg == 255)
		return;

	assert(registerUsers[reg] > 0);

	registerUsers[reg]--;

	if(registerUsers[reg] == 0)
		freedRegisters.push_back(reg);
}

void RegVmLoweredFunction::CompleteUse(VmValue *value)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	assert(instruction);
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

	// Temp
	if(instruction->regVmRegisters.empty())
		return 255;

	return instruction->regVmRegisters[0];
}

void RegVmLoweredFunction::GetRegisters(SmallArray<unsigned char, 8> &result, VmValue *value)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	CompleteUse(value);

	// Temp
	if(instruction->regVmRegisters.empty())
	{
		result.push_back(255);
		result.push_back(255);
		result.push_back(255);
		result.push_back(255);

		return;
	}

	for(unsigned i = 0; i < instruction->regVmRegisters.size(); i++)
		result.push_back(instruction->regVmRegisters[i]);
}

unsigned char RegVmLoweredFunction::AllocateRegister(VmValue *value, bool additional)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	assert(instruction);
	assert(!instruction->users.empty());

	if(!additional)
		assert(instruction->regVmRegisters.empty());

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

bool RegVmLoweredFunction::TransferRegisterTo(VmValue *value, unsigned char reg)
{
	VmInstruction *instruction = getType<VmInstruction>(value);

	assert(instruction);

	// Temp
	if(reg == 255)
	{
		instruction->regVmRegisters.push_back(reg);

		return true;
	}

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

void LowerConstantIntoBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, SmallArray<unsigned char, 8> &result, VmValue *value)
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
		unsigned data[2];
		memcpy(data, &constant->dValue, 8);

		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		lowBlock->AddInstruction(ctx, constant->source, rviLoadImm, targetReg, 0, 0, data[0]);
		lowBlock->AddInstruction(ctx, constant->source, rviLoadImmHigh, targetReg, 0, 0, data[1]);

		result.push_back(targetReg);
	}
	else if(constant->type == VmType::Long)
	{
		unsigned data[2];
		memcpy(data, &constant->lValue, 8);

		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		lowBlock->AddInstruction(ctx, constant->source, rviLoadImm, targetReg, 0, 0, data[0]);
		lowBlock->AddInstruction(ctx, constant->source, rviLoadImmHigh, targetReg, 0, 0, data[1]);

		result.push_back(targetReg);
	}
	else if(constant->type.type == VM_TYPE_POINTER)
	{
		unsigned char targetReg = lowFunction->GetRegisterForConstant();

		if(!constant->container)
		{
			assert(constant->iValue == 0);

			lowBlock->AddInstruction(ctx, constant->source, rviLoadImm, targetReg, 0, 0, 0u);
		}
		else
		{
			lowBlock->AddInstruction(ctx, constant->source, rviGetAddr, targetReg, IsLocalScope(constant->container->scope), 0, constant);
		}

		result.push_back(targetReg);
	}
	else if(constant->type.type == VM_TYPE_STRUCT)
	{
		assert(constant->type.size % 4 == 0);

		bool subPos = false;

		for(int i = int(constant->type.size / 4) - 1; i >= 0; i--)
		{
			if(subPos)
			{
				unsigned char targetReg = result.back();

				lowBlock->AddInstruction(ctx, constant->source, rviLoadImmHigh, targetReg, 0, 0, ((unsigned*)constant->sValue)[i]);
			}
			else
			{
				unsigned char targetReg = lowFunction->GetRegisterForConstant();
				result.push_back(targetReg);

				lowBlock->AddInstruction(ctx, constant->source, rviLoadImm, targetReg, 0, 0, ((unsigned*)constant->sValue)[i]);
			}

			subPos = !subPos;
		}
	}
	else
	{
		assert(!"unknown type");
	}
}

unsigned char GetArgumentRegister(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmValue *value)
{
	if(VmConstant *constant = getType<VmConstant>(value))
	{
		SmallArray<unsigned char, 8> result;

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

void GetArgumentRegisters(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, SmallArray<unsigned char, 8> &result, VmValue *value)
{
	if(VmConstant *constant = getType<VmConstant>(value))
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

void LowerInstructionIntoBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, RegVmLoweredBlock *lowBlock, VmValue *value)
{
	VmInstruction *inst = getType<VmInstruction>(value);

	assert(inst);

	switch(inst->cmd)
	{
	case VM_INST_LOAD_BYTE:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadByte, targetReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadBytePtr, targetReg, 0, sourceReg, offset->iValue);
		}
		break;
	case VM_INST_LOAD_SHORT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadWord, targetReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadWordPtr, targetReg, 0, sourceReg, offset->iValue);
		}
		break;
	case VM_INST_LOAD_INT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadDword, targetReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadDwordPtr, targetReg, 0, sourceReg, offset->iValue);
		}
		break;
	case VM_INST_LOAD_FLOAT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadFloat, targetReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadFloatPtr, targetReg, 0, sourceReg, offset->iValue);
		}
		break;
	case VM_INST_LOAD_DOUBLE:
	case VM_INST_LOAD_LONG:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadQword, targetReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadQwordPtr, targetReg, 0, sourceReg, offset->iValue);
		}
		break;
	/*case VM_INST_LOAD_STRUCT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

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
		break;*/
	case VM_INST_LOAD_IMMEDIATE:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviLoadImm, targetReg, 0, 0, constant);
		}
		break;
	case VM_INST_STORE_BYTE:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreByte, sourceReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreBytePtr, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_SHORT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreWord, sourceReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreWordPtr, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_INT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreDword, sourceReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_FLOAT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreFloat, sourceReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreFloatPtr, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_LONG:
	case VM_INST_STORE_DOUBLE:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreQword, sourceReg, IsLocalScope(constant->container->scope) ? 1 : 0, 0, constant);
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			unsigned char sourceReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[2]);
			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceReg, 0, addressReg, offset);
		}
		break;
	case VM_INST_STORE_STRUCT:
		if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			(void)offset;
			assert(offset->iValue == 0);

			SmallArray<unsigned char, 8> sourceRegs;
			GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[2]);

			assert((unsigned short)inst->arguments[2]->type.size == inst->arguments[2]->type.size);

			unsigned char flag = IsLocalScope(constant->container->scope) ? 1 : 0;

			unsigned pos = offset->iValue;

			if(inst->arguments[2]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[2]->type.type == VM_TYPE_ARRAY_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[0], flag, 0, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], flag, 0, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], flag, 0, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], flag, 0, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], flag, 0, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[1], flag, 0, pos);
					pos += 8;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], flag, 0, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], flag, 0, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_ARRAY)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], flag, 0, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[1], flag, 0, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[2], flag, 0, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], flag, 0, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], flag, 0, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[2], flag, 0, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_STRUCT)
			{
				unsigned remainingSize = inst->arguments[2]->type.size;

				assert(remainingSize % 4 == 0);

				for(unsigned i = 0; i < sourceRegs.size(); i++)
				{
					if(remainingSize == 8)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[i], flag, 0, pos);
						pos += 8;
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[i], flag, 0, pos);
						pos += 4;
					}

					remainingSize -= 8;
				}
			}
		}
		else
		{
			VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

			SmallArray<unsigned char, 8> sourceRegs;
			GetArgumentRegisters(ctx, lowFunction, lowBlock, sourceRegs, inst->arguments[2]);

			unsigned char addressReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			assert((unsigned short)inst->arguments[2]->type.size == inst->arguments[2]->type.size);

			unsigned pos = offset->iValue;

			if(inst->arguments[2]->type.type == VM_TYPE_FUNCTION_REF || inst->arguments[2]->type.type == VM_TYPE_ARRAY_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[0], 0, addressReg, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], 0, addressReg, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_REF)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[1], 0, addressReg, pos);
					pos += 8;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], 0, addressReg, pos);
					pos += 4;
				}
			}
			else if(inst->arguments[2]->type.type == VM_TYPE_AUTO_ARRAY)
			{
				if(NULLC_PTR_SIZE == 8)
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[1], 0, addressReg, pos);
					pos += 8;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[2], 0, addressReg, pos);
					pos += 4;
				}
				else
				{
					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[0], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[1], 0, addressReg, pos);
					pos += 4;

					lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[2], 0, addressReg, pos);
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
						lowBlock->AddInstruction(ctx, inst->source, rviStoreDwordPtr, sourceRegs[i], 0, addressReg, pos);
						pos += 4;
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviStoreQwordPtr, sourceRegs[i], 0, addressReg, pos);
						pos += 8;
					}
					
					remainingSize -= 8;
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
	/*case VM_INST_INDEX:
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
	break;*/
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
	/*case VM_INST_SET_RANGE:
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
		break;*/
	case VM_INST_JUMP:
		// Check if jump is fall-through
		if(!(lowBlock->vmBlock->nextSibling && lowBlock->vmBlock->nextSibling == inst->arguments[0]))
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
			lowBlock->AddInstruction(ctx, inst->source, rviJmpnz, sourceReg, 0, 0, getType<VmBlock>(inst->arguments[2]));
		}
		else
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmpz, sourceReg, 0, 0, getType<VmBlock>(inst->arguments[1]));

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
			lowBlock->AddInstruction(ctx, inst->source, rviJmpz, sourceReg, 0, 0, getType<VmBlock>(inst->arguments[2]));
		}
		else
		{
			lowBlock->AddInstruction(ctx, inst->source, rviJmpnz, sourceReg, 0, 0, getType<VmBlock>(inst->arguments[1]));

			lowBlock->AddInstruction(ctx, inst->source, rviJmp, 0, 0, 0, getType<VmBlock>(inst->arguments[2]));
		}
	}
		break;
	case VM_INST_CALL:
	{
		VmInstruction *target = getType<VmInstruction>(inst->arguments[0]);

		assert(target);

		assert((unsigned short)inst->type.size == inst->type.size);

		SmallArray<unsigned char, 8> targetRegs;
		GetArgumentRegisters(ctx, lowFunction, lowBlock, targetRegs, target);

		if(NULLC_PTR_SIZE == 8)
			lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, targetRegs[0]);
		else
			lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, targetRegs[0]);

		for(int i = int(inst->arguments.size() - 1); i >= 1; i--)
		{
			VmValue *argument = inst->arguments[i];

			if(argument->type.size == 0)
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, 0); // Don't care about 0 register size
			}
			else
			{
				SmallArray<unsigned char, 8> argumentRegs;
				GetArgumentRegisters(ctx, lowFunction, lowBlock, argumentRegs, argument);

				if(argument->type.type == VM_TYPE_INT || (NULLC_PTR_SIZE == 4 && argument->type.type == VM_TYPE_POINTER))
				{
					lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
				}
				else if(argument->type.type == VM_TYPE_DOUBLE || argument->type.type == VM_TYPE_LONG || (NULLC_PTR_SIZE == 8 && argument->type.type == VM_TYPE_POINTER))
				{
					lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[0]);
				}
				else if(argument->type.type == VM_TYPE_FUNCTION_REF || argument->type.type == VM_TYPE_ARRAY_REF)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[0]);
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
					}
				}
				else if(argument->type.type == VM_TYPE_AUTO_REF)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
						lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[1]);
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
					}
				}
				else if(argument->type.type == VM_TYPE_AUTO_ARRAY)
				{
					if(NULLC_PTR_SIZE == 8)
					{
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
						lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[1]);
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[2]);
					}
					else
					{
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
						lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[2]);
					}
				}
				else if(argument->type.type == VM_TYPE_STRUCT)
				{
					unsigned remainingSize = argument->type.size;

					for(unsigned i = 0; i < argumentRegs.size(); i++)
					{
						if(remainingSize == 8)
							lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[i]);
						else
							lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[i]);

						remainingSize -= 8;
					}
				}
			}
		}

		if(inst->type.type == VM_TYPE_VOID || inst->users.empty())
		{
			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, rvrVoid, targetRegs[1]);
		}
		else if(inst->type.type == VM_TYPE_INT || (NULLC_PTR_SIZE == 4 && inst->type.type == VM_TYPE_POINTER))
		{
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, targetReg, rvrInt, targetRegs[1]);
		}
		else if(inst->type.type == VM_TYPE_DOUBLE || inst->type.type == VM_TYPE_LONG || (NULLC_PTR_SIZE == 8 && inst->type.type == VM_TYPE_POINTER))
		{
			unsigned char targetReg = lowFunction->AllocateRegister(inst);

			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, targetReg, (unsigned char)(inst->type.type == VM_TYPE_DOUBLE ? rvrDouble : rvrLong), targetRegs[1]);
		}
		else if(inst->type.type == VM_TYPE_FUNCTION_REF || inst->type.type == VM_TYPE_ARRAY_REF)
		{
			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1]);
			/*if(NULLC_PTR_SIZE == 8)
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[0]);
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
			}*/
		}
		else if(inst->type.type == VM_TYPE_AUTO_REF)
		{
			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1]);
			/*if(NULLC_PTR_SIZE == 8)
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
				lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[1]);
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
			}*/
		}
		else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
		{
			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1]);
			/*if(NULLC_PTR_SIZE == 8)
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
				lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[1]);
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[2]);
			}
			else
			{
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[0]);
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[1]);
				lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[2]);
			}*/
		}
		else if(inst->type.type == VM_TYPE_STRUCT)
		{
			lowBlock->AddInstruction(ctx, inst->source, rviCallPtr, 0, 0, targetRegs[1]);
			/*unsigned remainingSize = argument->type.size;

			for(unsigned i = 0; i < argumentRegs.size(); i++)
			{
				if(remainingSize == 8)
					lowBlock->AddInstruction(ctx, inst->source, rviPushq, 0, 0, argumentRegs[i]);
				else
					lowBlock->AddInstruction(ctx, inst->source, rviPush, 0, 0, argumentRegs[i]);

				remainingSize -= 8;
			}*/
		}
	}
	break;
	/*case VM_INST_RETURN:
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
	break;*/
	case VM_INST_ADD:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviAdd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviAddd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviAddl, targetReg, lhsReg, rhsReg);
		else if(inst->type.type == VM_TYPE_POINTER)
			lowBlock->AddInstruction(ctx, inst->source, NULLC_PTR_SIZE == 4 ? rviAdd : rviAddl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
	break;
	case VM_INST_SUB:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviSub, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviSubd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviSubl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
	break;
	case VM_INST_MUL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviMul, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviMuld, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviMull, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_DIV:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviDiv, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviDivd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviDivl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_POW:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviPow, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviPowd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviPowl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_MOD:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviMod, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviModd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviModl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_LESS:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviLess, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviLessd, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviLessl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_GREATER:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviGreater, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviGreaterd, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviGreaterl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_LESS_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviLequal, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviLequald, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviLequal, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_GREATER_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviGequal, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviGequald, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviGequall, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviEqual, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviEquald, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviEquall, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
			lowBlock->AddInstruction(ctx, inst->source, NULLC_PTR_SIZE == 4 ? rviEqual : rviEquall, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_NOT_EQUAL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviNequal, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Double)
			lowBlock->AddInstruction(ctx, inst->source, rviNequald, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviNequall, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
			lowBlock->AddInstruction(ctx, inst->source, NULLC_PTR_SIZE == 4 ? rviNequal : rviNequall, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_SHL:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviShl, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviShll, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_SHR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviShr, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviShrl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_BIT_AND:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviBitAnd, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviBitAndl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_BIT_OR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviBitOr, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviBitOrl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_BIT_XOR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviBitXor, targetReg, lhsReg, rhsReg);
		else if(inst->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviBitXorl, targetReg, lhsReg, rhsReg);
		else
			assert(!"unknown type");
	}
		break;
	case VM_INST_LOG_XOR:
	{
		unsigned char lhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
		unsigned char rhsReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);
		unsigned char targetReg = lowFunction->AllocateRegister(inst);

		if(inst->arguments[0]->type == VmType::Int)
			lowBlock->AddInstruction(ctx, inst->source, rviLogXor, targetReg, lhsReg, rhsReg);
		else if(inst->arguments[0]->type == VmType::Long)
			lowBlock->AddInstruction(ctx, inst->source, rviLogXorl);
		else
			assert(!"unknown type");
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
	/*case VM_INST_CONVERT_POINTER:
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
		break;*/
	case VM_INST_CONSTRUCT:
		if(inst->type.type == VM_TYPE_FUNCTION_REF)
		{
			unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char idReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, ptrReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, idReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, idReg);
			}
		}
		else if(inst->type.type == VM_TYPE_ARRAY_REF)
		{
			unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char lenReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, ptrReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, lenReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, lenReg);
			}
		}
		else if(inst->type.type == VM_TYPE_AUTO_REF)
		{
			unsigned char typeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);
			unsigned char ptrReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, typeReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, typeReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, ptrReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, ptrReg);
			}
		}
		else if(inst->type.type == VM_TYPE_AUTO_ARRAY)
		{
			unsigned char typeReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[0]);

			SmallArray<unsigned char, 8> arrayRegs;
			GetArgumentRegisters(ctx, lowFunction, lowBlock, arrayRegs, inst->arguments[1]);

			if(!lowFunction->TransferRegisterTo(inst, typeReg))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, typeReg);
			}

			if(!lowFunction->TransferRegisterTo(inst, arrayRegs[0]))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, arrayRegs[0]);
			}

			if(!lowFunction->TransferRegisterTo(inst, arrayRegs[1]))
			{
				unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

				lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, arrayRegs[1]);
			}
		}
		else
		{
			assert(!"unknown type");
		}
		break;
	case VM_INST_ARRAY:
	{
		assert(inst->arguments.size() <= 8);

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

					unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

					lowBlock->AddInstruction(ctx, inst->source, rviCombinedd, copyReg, argReg1, argReg2);

					i++;
				}
				else
				{
					unsigned char argReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[i]);

					if(!lowFunction->TransferRegisterTo(inst, argReg))
					{
						unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

						lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, argReg);
					}
				}
			}
			else if(argument->type.size == 8)
			{
				unsigned char argReg = GetArgumentRegister(ctx, lowFunction, lowBlock, inst->arguments[i]);

				if(!lowFunction->TransferRegisterTo(inst, argReg))
				{
					unsigned char copyReg = lowFunction->AllocateRegister(inst, true);

					lowBlock->AddInstruction(ctx, inst->source, rviMov, copyReg, 0, argReg);
				}
			}
			else
			{
				assert(!"unknown array element type size");
			}
		}
		break;
	}
	/*case VM_INST_EXTRACT:
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
		break;*/
	default:
		lowBlock->AddInstruction(ctx, inst->source, rviNop);

		//assert(!"unknown instruction");
	}

	lowFunction->FreeConstantRegisters();
}

RegVmLoweredBlock* RegVmLowerBlock(ExpressionContext &ctx, RegVmLoweredFunction *lowFunction, VmBlock *vmBlock)
{
	RegVmLoweredBlock *lowBlock = new (ctx.get<RegVmLoweredBlock>()) RegVmLoweredBlock(vmBlock);

	for(VmInstruction *vmInstruction = vmBlock->firstInstruction; vmInstruction; vmInstruction = vmInstruction->nextSibling)
	{
		LowerInstructionIntoBlock(ctx, lowFunction, lowBlock, vmInstruction);
	}

	return lowBlock;
}

RegVmLoweredFunction* RegVmLowerFunction(ExpressionContext &ctx, VmFunction *vmFunction)
{
	RegVmLoweredFunction *lowFunction = new (ctx.get<RegVmLoweredFunction>()) RegVmLoweredFunction(ctx.allocator, vmFunction);

	assert(vmFunction->firstBlock);

	for(VmBlock *vmBlock = vmFunction->firstBlock; vmBlock; vmBlock = vmBlock->nextSibling)
	{
		lowFunction->blocks.push_back(RegVmLowerBlock(ctx, lowFunction, vmBlock));
	}

	return lowFunction;
}

RegVmLoweredModule* RegVmLowerModule(ExpressionContext &ctx, VmModule *vmModule)
{
	RegVmLoweredModule *lowModule = new (ctx.get<RegVmLoweredModule>()) RegVmLoweredModule(ctx.allocator, vmModule);

	for(VmFunction *vmFunction = vmModule->functions.head; vmFunction; vmFunction = vmFunction->next)
	{
		if(vmFunction->function && vmFunction->function->importModule != NULL)
			continue;

		if(vmFunction->function && vmFunction->function->isPrototype && !vmFunction->function->implementation)
			continue;

		lowModule->functions.push_back(RegVmLowerFunction(ctx, vmFunction));
	}

	return lowModule;
}

void FinalizeInstruction(InstructionRegVmFinalizeContext &ctx, RegVmLoweredInstruction *lowInstruction)
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
}

void FinalizeBlock(InstructionRegVmFinalizeContext &ctx, RegVmLoweredBlock *lowBlock)
{
	lowBlock->vmBlock->address = ctx.cmds.size();

	for(RegVmLoweredInstruction *curr = lowBlock->firstInstruction; curr; curr = curr->nextSibling)
	{
		FinalizeInstruction(ctx, curr);
	}
}

void FinalizeFunction(InstructionRegVmFinalizeContext &ctx, RegVmLoweredFunction *lowFunction)
{
	lowFunction->vmFunction->address = ctx.cmds.size();

	if(FunctionData *data = lowFunction->vmFunction->function)
	{
		assert(data->argumentsSize < 65536);

		// Stack frame should remain aligned, so its size should multiple of 16
		unsigned size = (data->stackSize + 0xf) & ~0xf;

		// Save previous stack frame, and expand current by shift bytes
		ctx.locations.push_back(data->source);
		ctx.cmds.push_back(RegVmCmd(rviPushvtop, 0, (unsigned char)(data->argumentsSize >> 8), (unsigned char)(data->argumentsSize & 0xff), size));
	}

	for(unsigned i = 0; i < lowFunction->blocks.size(); i++)
	{
		RegVmLoweredBlock *lowBlock = lowFunction->blocks[i];

		FinalizeBlock(ctx, lowBlock);
	}

	for(unsigned i = 0; i < ctx.fixupPoints.size(); i++)
	{
		InstructionRegVmFinalizeContext::FixupPoint &point = ctx.fixupPoints[i];

		assert(point.target);
		assert(point.target->address != ~0u);

		ctx.cmds[point.cmdIndex].argument = point.target->address;
	}

	ctx.fixupPoints.clear();

	lowFunction->vmFunction->codeSize = ctx.cmds.size() - lowFunction->vmFunction->address;

	ctx.currentFunction = NULL;
}

void FinalizeModule(InstructionRegVmFinalizeContext &ctx, RegVmLoweredModule *lowModule)
{
	ctx.locations.push_back(NULL);
	ctx.cmds.push_back(RegVmCmd(rviJmp, 0, 0, 0, 0));

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
	{
		RegVmLoweredFunction *lowFunction = lowModule->functions[i];

		if(!lowFunction->vmFunction->function)
		{
			lowModule->vmModule->globalCodeStart = ctx.cmds.size();

			ctx.cmds[0].argument = lowModule->vmModule->globalCodeStart;
		}

		FinalizeFunction(ctx, lowFunction);
	}
}