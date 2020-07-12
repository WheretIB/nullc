#include "InstructionTreeRegVmLowerGraph.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"
#include "InstructionTreeRegVmLower.h"
#include "Bytecode.h"

void (*nullcDumpGraphRegVmLoweredModule)(RegVmLoweredModule*) = DumpGraph;

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

NULLC_PRINT_FORMAT_CHECK(2, 3) void Print(OutputContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.Print(format, args);

	va_end(args);
}

void PrintIndent(OutputContext &ctx)
{
	ctx.Print("  ");
}

void PrintLine(OutputContext &ctx)
{
	ctx.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(2, 3) void PrintLine(OutputContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.Print(format, args);

	va_end(args);

	PrintLine(ctx);
}

void PrintRegister(OutputContext &ctx, unsigned char value)
{
	if(value == rvrrGlobals)
		Print(ctx, "rG");
	else if(value == rvrrFrame)
		Print(ctx, "rF");
	else if(value == rvrrConstants)
		Print(ctx, "rC");
	else if(value == rvrrRegisters)
		Print(ctx, "rR");
	else
		Print(ctx, "r%d", value);
}

void PrintConstant(OutputContext &ctx, VmConstant *constant)
{
	if(constant->type == VmType::Void)
		Print(ctx, "{}");
	else if(constant->type == VmType::Int)
		Print(ctx, "%d", constant->iValue);
	else if(constant->type == VmType::Double)
		Print(ctx, "%f", constant->dValue);
	else if(constant->type == VmType::Long)
		Print(ctx, "%lldl", constant->lValue);
	else if(constant->type.type == VM_TYPE_POINTER && constant->container && constant->iValue)
		Print(ctx, "%.*s.v%04x+0x%x%s", FMT_ISTR(constant->container->name->name), constant->container->uniqueId, constant->iValue, HasAddressTaken(constant->container) ? "" : " (noalias)");
	else if(constant->type.type == VM_TYPE_POINTER && constant->container)
		Print(ctx, "%.*s.v%04x%s", FMT_ISTR(constant->container->name->name), constant->container->uniqueId, HasAddressTaken(constant->container) ? "" : " (noalias)");
	else if(constant->type.type == VM_TYPE_POINTER)
		Print(ctx, "0x%x", constant->iValue);
	else if(constant->type.type == VM_TYPE_STRUCT)
		Print(ctx, "{ %.*s }", FMT_ISTR(constant->type.structType->name));
	else if(constant->type.type == VM_TYPE_BLOCK)
		Print(ctx, "%.*s.b%d", FMT_ISTR(constant->bValue->name), constant->bValue->uniqueId);
	else if(constant->type.type == VM_TYPE_FUNCTION && constant->fValue->function)
		Print(ctx, "%.*s.f%04x", FMT_ISTR(constant->fValue->function->name->name), constant->fValue->function->uniqueId);
	else if(constant->type.type == VM_TYPE_FUNCTION)
		Print(ctx, "global.f0000");
	else
		assert(!"unknown type");
}

void PrintConstant(OutputContext &ctx, unsigned argument, VmConstant *constant)
{
	if(constant)
		PrintConstant(ctx, constant);
	else
		Print(ctx, "%d", argument);
}

void PrintAddress(OutputContext &ctx, char *constantData, unsigned char rC, unsigned argument, VmConstant *constant, VmValueType memoryType)
{
	if(rC == rvrrConstants && constantData)
	{
		if(memoryType == VM_TYPE_INT || (memoryType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 4))
		{
			int value = 0;
			memcpy(&value, constantData + argument, sizeof(value));
			Print(ctx, "%d @%d", value, argument);
		}
		else if(memoryType == VM_TYPE_DOUBLE)
		{
			double value = 0.0;
			memcpy(&value, constantData + argument, sizeof(value));
			Print(ctx, "%f @%d", value, argument);
		}
		else if(memoryType == VM_TYPE_LONG || (memoryType == VM_TYPE_POINTER && NULLC_PTR_SIZE == 8))
		{
			long long value = 0ll;
			memcpy(&value, constantData + argument, sizeof(value));
			Print(ctx, "%lldl @%d", value, argument);
		}
		else
		{
			Print(ctx, "@%d", argument);
		}
	}
	else if(rC == rvrrRegisters)
	{
		Print(ctx, "r%d", int(argument / sizeof(RegVmRegister)));
	}
	else
	{
		Print(ctx, "[");
		PrintRegister(ctx, rC);
		Print(ctx, " + ");

		if(constant)
			PrintConstant(ctx, constant);
		else
			Print(ctx, "%d", argument);

		Print(ctx, "]");
	}
}

void PrintCall(OutputContext &ctx, char *constantData, unsigned microcodePos)
{
	Print(ctx, "(");

	unsigned *microcode = (unsigned*)constantData + microcodePos;

	while(*microcode != rvmiCall)
	{
		switch(*microcode++)
		{
		case rvmiPush:
			Print(ctx, "r%d~", *microcode++);
			break;
		case rvmiPushQword:
			Print(ctx, "r%d", *microcode++);
			break;
		case rvmiPushImm:
			Print(ctx, "%d", *microcode++);
			break;
		case rvmiPushImmq:
			Print(ctx, "%dL", *microcode++);
			break;
		case rvmiPushMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			Print(ctx, "[");
			PrintRegister(ctx, (unsigned char)reg);
			Print(ctx, " + 0x%x] x%d", offset, size);
		}
		break;
		default:
			assert(!"unknown micro instruction");
		}

		if(*microcode != rvmiCall)
			Print(ctx, ", ");
	}

	Print(ctx, ") -> ");

	microcode++;

	unsigned targetReg = *microcode++;
	unsigned resultType = *microcode++;

	switch(resultType)
	{
	case rvrVoid:
		Print(ctx, "void");
		break;
	case rvrDouble:
		Print(ctx, "double (r%d", targetReg);
		break;
	case rvrLong:
		Print(ctx, "long (r%d", targetReg);
		break;
	case rvrInt:
		Print(ctx, "int (r%d", targetReg);
		break;
	case rvrStruct:
		Print(ctx, "struct (");
		break;
	case rvrError:
		Print(ctx, "error");
		break;
	default:
		assert(!"unknown type");
	}

	while(*microcode != rvmiReturn)
	{
		switch(*microcode++)
		{
		case rvmiPop:
			Print(ctx, "r%d~", *microcode++);
			break;
		case rvmiPopq:
			Print(ctx, "r%d", *microcode++);
			break;
		case rvmiPopMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			Print(ctx, "[");
			PrintRegister(ctx, (unsigned char)reg);
			Print(ctx, " + 0x%x] x%d", offset, size);
		}
		break;
		default:
			assert(!"unknown micro instruction");
		}

		if(*microcode != rvmiReturn)
			Print(ctx, ", ");
	}

	if(resultType == rvrVoid || resultType == rvrError)
		Print(ctx, " @%d", microcodePos * 4);
	else
		Print(ctx, ") @%d", microcodePos * 4);
}

void PrintReturn(OutputContext &ctx, char *constantData, unsigned microcodePos)
{
	unsigned *microcode = (unsigned*)constantData + microcodePos;

	unsigned typeId = *microcode++;
	unsigned typeSize = *microcode++;

	Print(ctx, " (%d, %d) -> (", typeId, typeSize);

	while(*microcode != rvmiReturn)
	{
		switch(*microcode++)
		{
		case rvmiPush:
			Print(ctx, "r%d~", *microcode++);
			break;
		case rvmiPushQword:
			Print(ctx, "r%d", *microcode++);
			break;
		case rvmiPushImm:
			Print(ctx, "%d", *microcode++);
			break;
		case rvmiPushImmq:
			Print(ctx, "%dL", *microcode++);
			break;
		case rvmiPushMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			Print(ctx, "[");
			PrintRegister(ctx, (unsigned char)reg);
			Print(ctx, " + 0x%x] x%d", offset, size);
		}
		break;
		default:
			assert(!"unknown micro instruction");
		}

		if(*microcode != rvmiReturn)
			Print(ctx, ", ");
	}

	Print(ctx, ") @%d", microcodePos * 4);
}

void PrintInstruction(OutputContext &ctx, char *constantData, ExternFuncInfo *functionData, char *symbolData, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument, VmConstant *constant)
{
	Print(ctx, "%s ", GetInstructionName(code));

	switch(code)
	{
	case rviNop:
		break;
	case rviLoadByte:
	case rviLoadWord:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_VOID);
		break;
	case rviLoadDword:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_INT);
		break;
	case rviLoadLong:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_LONG);
		break;
	case rviLoadFloat:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_VOID);
		break;
	case rviLoadDouble:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_DOUBLE);
		break;
	case rviLoadImm:
	case rviLoadImmLong:
	case rviLoadImmDouble:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviStoreByte:
	case rviStoreWord:
	case rviStoreDword:
	case rviStoreLong:
	case rviStoreFloat:
	case rviStoreDouble:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_VOID);
		break;
	case rviCombinedd:
	case rviBreakupdd:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		break;
	case rviMov:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);

		switch(rB)
		{
		case rvcFull:
			Print(ctx, " (full)");
			break;
		case rvcInt:
			Print(ctx, " (int)");
			break;
		case rvcDouble:
			Print(ctx, " (double)");
			break;
		case rvcLong:
			Print(ctx, " (long)");
			break;
		default:
			assert(!"unknown type");
		}
		break;
	case rviMovMult:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);

		switch(rB & 0x3)
		{
		case rvcFull:
			Print(ctx, " (full)");
			break;
		case rvcInt:
			Print(ctx, " (int)");
			break;
		case rvcDouble:
			Print(ctx, " (double)");
			break;
		case rvcLong:
			Print(ctx, " (long)");
			break;
		default:
			assert(!"unknown type");
		}

		if(argument >> 24)
		{
			Print(ctx, "; ");
			PrintRegister(ctx, argument >> 24);
			Print(ctx, ", ");
			PrintRegister(ctx, (argument >> 16) & 0xff);

			switch((rB >> 2) & 0x3)
			{
			case rvcFull:
				Print(ctx, " (full)");
				break;
			case rvcInt:
				Print(ctx, " (int)");
				break;
			case rvcDouble:
				Print(ctx, " (double)");
				break;
			case rvcLong:
				Print(ctx, " (long)");
				break;
			default:
				assert(!"unknown type");
			}
		}

		if((argument >> 8) & 0xff)
		{
			Print(ctx, "; ");
			PrintRegister(ctx, (argument >> 8) & 0xff);
			Print(ctx, ", ");
			PrintRegister(ctx, argument & 0xff);

			switch((rB >> 4) & 0x3)
			{
			case rvcFull:
				Print(ctx, " (full)");
				break;
			case rvcInt:
				Print(ctx, " (int)");
				break;
			case rvcDouble:
				Print(ctx, " (double)");
				break;
			case rvcLong:
				Print(ctx, " (long)");
				break;
			default:
				assert(!"unknown type");
			}
		}
		break;
	case rviDtoi:
	case rviDtol:
	case rviDtof:
	case rviItod:
	case rviLtod:
	case rviItol:
	case rviLtoi:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		break;
	case rviIndex:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		Print(ctx, ", ");

		if(constant)
		{
			PrintRegister(ctx, (constant->iValue >> 16) & 0xff);
			Print(ctx, ", %d", constant->iValue & 0xffff);
		}
		else
		{
			PrintRegister(ctx, (argument >> 16) & 0xff);
			Print(ctx, ", %d", argument & 0xffff);
		}
		break;
	case rviGetAddr:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_VOID);
		break;
	case rviSetRange:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");

		switch(rB)
		{
		case rvsrDouble:
			Print(ctx, "double");
			break;
		case rvsrFloat:
			Print(ctx, "float");
			break;
		case rvsrLong:
			Print(ctx, "long");
			break;
		case rvsrInt:
			Print(ctx, "int");
			break;
		case rvsrShort:
			Print(ctx, "short");
			break;
		case rvsrChar:
			Print(ctx, "char");
			break;
		default:
			assert(!"unknown type");
		}

		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviMemCopy:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviJmp:
		PrintConstant(ctx, argument, constant);
		break;
	case rviJmpz:
	case rviJmpnz:
		PrintRegister(ctx, rC);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviCall:
		if(constant)
			PrintConstant(ctx, constant);
		else if (functionData)
			Print(ctx, "%s#%d", functionData[argument].offsetToName + symbolData, argument);
		else
			Print(ctx, "%d", argument);

		PrintCall(ctx, constantData, (rA << 16) | (rB << 8) | rC);
		break;
	case rviCallPtr:
		PrintRegister(ctx, rC);
		PrintCall(ctx, constantData, argument);
		break;
	case rviReturn:
		if(rC)
			Print(ctx, "checked ");

		switch(rB)
		{
		case rvrVoid:
			Print(ctx, "void");
			break;
		case rvrDouble:
			Print(ctx, "double");
			break;
		case rvrLong:
			Print(ctx, "long");
			break;
		case rvrInt:
			Print(ctx, "int");
			break;
		case rvrStruct:
			Print(ctx, "struct");
			break;
		case rvrError:
			Print(ctx, "error");
			break;
		default:
			assert(!"unknown type");
		}

		if(rB != rvrError)
			PrintReturn(ctx, constantData, argument);
		break;
	case rviAddImm:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviAdd:
	case rviSub:
	case rviMul:
	case rviDiv:
	case rviPow:
	case rviMod:
	case rviLess:
	case rviGreater:
	case rviLequal:
	case rviGequal:
	case rviEqual:
	case rviNequal:
	case rviShl:
	case rviShr:
	case rviBitAnd:
	case rviBitOr:
	case rviBitXor:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_INT);
		break;
	case rviAddImml:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviAddl:
	case rviSubl:
	case rviMull:
	case rviDivl:
	case rviPowl:
	case rviModl:
	case rviLessl:
	case rviGreaterl:
	case rviLequall:
	case rviGequall:
	case rviEquall:
	case rviNequall:
	case rviShll:
	case rviShrl:
	case rviBitAndl:
	case rviBitOrl:
	case rviBitXorl:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_LONG);
		break;
	case rviAddd:
	case rviSubd:
	case rviMuld:
	case rviDivd:
	case rviAddf:
	case rviSubf:
	case rviMulf:
	case rviDivf:
	case rviPowd:
	case rviModd:
	case rviLessd:
	case rviGreaterd:
	case rviLequald:
	case rviGequald:
	case rviEquald:
	case rviNequald:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintAddress(ctx, constantData, rC, argument, constant, VM_TYPE_DOUBLE);
		break;
	case rviNeg:
	case rviNegl:
	case rviNegd:
	case rviBitNot:
	case rviBitNotl:
	case rviLogNot:
	case rviLogNotl:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		break;
	case rviConvertPtr:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintRegister(ctx, rB);
		Print(ctx, ", ");
		PrintRegister(ctx, rC);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	case rviFuncAddr:
	case rviTypeid:
		PrintRegister(ctx, rA);
		Print(ctx, ", ");
		PrintConstant(ctx, argument, constant);
		break;
	default:
		assert(!"unknown instruction");
	}
}

void PrintInstruction(InstructionRegVmLowerGraphContext &ctx, char *constantData, RegVmLoweredInstruction *lowInstruction)
{
	if(ctx.showSource && lowInstruction->location && !lowInstruction->location->isInternal)
	{
		const char *start = lowInstruction->location->pos.begin;
		const char *end = start + 1;

		// TODO: handle source locations from imported modules
		while(start > ctx.code && *(start - 1) != '\r' && *(start - 1) != '\n')
			start--;

		while(*end && *end != '\r' && *end != '\n')
			end++;

		if (ctx.showAnnotatedSource)
		{
			unsigned startOffset = unsigned(lowInstruction->location->pos.begin - start);
			unsigned endOffset = unsigned(lowInstruction->location->pos.end - start);

			if(start != ctx.lastStart || startOffset != ctx.lastStartOffset || endOffset != ctx.lastEndOffset)
			{
				Print(ctx.output, "// %.*s", unsigned(end - start), start);
				PrintLine(ctx.output);
				PrintIndent(ctx.output);

				if (lowInstruction->location->pos.end < end)
				{
					Print(ctx.output, "// ");

					for (unsigned i = 0; i < startOffset; i++)
					{
						Print(ctx.output, " ");

						if (start[i] == '\t')
							Print(ctx.output, i == 0 ? "  " : "   ");
					}

					for (unsigned i = startOffset; i < endOffset; i++)
					{
						Print(ctx.output, "~");

						if (start[i] == '\t')
							Print(ctx.output, i == 0 ? "~~" : "~~~");
					}

					PrintLine(ctx.output);
					PrintIndent(ctx.output);
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
				Print(ctx.output, "// %.*s", unsigned(end - start), start);
				PrintLine(ctx.output);
				PrintIndent(ctx.output);

				ctx.lastStart = start;
			}
		}
	}

	PrintInstruction(ctx.output, constantData, NULL, NULL, RegVmInstructionCode(lowInstruction->code), lowInstruction->rA, lowInstruction->rB, lowInstruction->rC, lowInstruction->argument ? lowInstruction->argument->iValue : 0, lowInstruction->argument);

	if(!lowInstruction->preKillRegisters.empty() || !lowInstruction->postKillRegisters.empty())
	{
		Print(ctx.output, "\t// kill[");

		for(unsigned i = 0; i < lowInstruction->preKillRegisters.size(); i++)
		{
			if(i != 0)
				Print(ctx.output, ", ");

			Print(ctx.output, "r%d*", lowInstruction->preKillRegisters[i]);
		}

		for(unsigned i = 0; i < lowInstruction->postKillRegisters.size(); i++)
		{
			if(i != 0 || !lowInstruction->preKillRegisters.empty())
				Print(ctx.output, ", ");

			Print(ctx.output, "r%d", lowInstruction->postKillRegisters[i]);
		}

		Print(ctx.output, "]");
	}

	PrintLine(ctx.output);
}

bool IsBlockTerminator(RegVmLoweredInstruction *lowInstruction)
{
	switch(lowInstruction->code)
	{
	case rviJmp:
	case rviJmpz:
	case rviJmpnz:
	case rviReturn:
		return true;
	default:
		break;
	}

	return false;
}

void PrintBlock(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredModule *lowModule, RegVmLoweredBlock *lowblock)
{
	PrintLine(ctx.output, "%.*s.b%d:", FMT_ISTR(lowblock->vmBlock->name), lowblock->vmBlock->uniqueId);

	if(!lowblock->entryRegisters.empty())
	{
		Print(ctx.output, "  // entry registers: [");

		for(unsigned i = 0; i < lowblock->entryRegisters.size(); i++)
		{
			if(i != 0)
				Print(ctx.output, ", ");

			Print(ctx.output, "r%d", lowblock->entryRegisters[i]);
		}

		PrintLine(ctx.output, "]");
	}

	if(!lowblock->reservedRegisters.empty())
	{
		Print(ctx.output, "  // reserved registers: [");

		for(unsigned i = 0; i < lowblock->reservedRegisters.size(); i++)
		{
			if(i != 0)
				Print(ctx.output, ", ");

			Print(ctx.output, "r%d", lowblock->reservedRegisters[i]);
		}

		PrintLine(ctx.output, "]");
	}

	for(RegVmLoweredInstruction *lowInstruction = lowblock->firstInstruction; lowInstruction; lowInstruction = lowInstruction->nextSibling)
	{
		PrintIndent(ctx.output);
		PrintInstruction(ctx, (char*)lowModule->constants.data, lowInstruction);
	}

	if(!lowblock->exitRegisters.empty())
	{
		Print(ctx.output, "  // exit registers: [");

		for(unsigned i = 0; i < lowblock->exitRegisters.size(); i++)
		{
			if(i != 0)
				Print(ctx.output, ", ");

			Print(ctx.output, "r%d", lowblock->exitRegisters[i]);
		}

		PrintLine(ctx.output, "]");
	}

	if(lowblock->lastInstruction && !IsBlockTerminator(lowblock->lastInstruction))
		PrintLine(ctx.output, "  // fallthrough");

	PrintLine(ctx.output);
}

void PrintFunction(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredModule *lowModule, RegVmLoweredFunction *lowFunction)
{
	if(FunctionData *fData = lowFunction->vmFunction->function)
	{
		Print(ctx.output, "function %.*s.f%04x(", FMT_ISTR(fData->name->name), fData->uniqueId);

		for(unsigned i = 0; i < fData->arguments.size(); i++)
		{
			ArgumentData &argument = fData->arguments[i];

			Print(ctx.output, "%s%s%.*s %.*s", i == 0 ? "" : ", ", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name), FMT_ISTR(argument.name->name));
		}

		PrintLine(ctx.output, ")");

		PrintLine(ctx.output, "// argument size %lld", fData->argumentsSize);
		PrintLine(ctx.output, "// stack size %lld", fData->stackSize);
		PrintLine(ctx.output, "// register count %d (r0-r%d)", fData->vmFunction->regVmRegisters, fData->vmFunction->regVmRegisters - 1);
	}
	else
	{
		PrintLine(ctx.output, "function global()");
	}

	if(ScopeData *scope = lowFunction->vmFunction->scope)
	{
		for(unsigned i = 0; i < scope->allVariables.size(); i++)
		{
			VariableData *variable = scope->allVariables[i];

			if(variable->isAlloca && variable->users.empty())
				continue;

			Print(ctx.output, "// %s0x%x: %.*s %.*s", variable->importModule ? "imported " : "", variable->offset, FMT_ISTR(variable->type->name), FMT_ISTR(variable->name->name));

			bool addressTaken = false;

			for(unsigned i = 0; i < variable->users.size(); i++)
			{
				VmConstant *user = variable->users[i];

				for(unsigned k = 0; k < user->users.size(); k++)
				{
					if(VmInstruction *inst = getType<VmInstruction>(user->users[k]))
					{
						bool simpleUse = false;

						if(inst->cmd >= VM_INST_LOAD_BYTE && inst->cmd <= VM_INST_LOAD_STRUCT)
							simpleUse = true;
						else if(inst->cmd >= VM_INST_STORE_BYTE && inst->cmd <= VM_INST_STORE_STRUCT && inst->arguments[0] == user)
							simpleUse = true;
						else
							simpleUse = false;

						if(!simpleUse)
							addressTaken = true;
					}
					else
					{
						assert(!"invalid constant use");
					}
				}
			}

			if(!addressTaken)
				Print(ctx.output, " noalias");

			if(variable->isAlloca)
				Print(ctx.output, " alloca");

			if(variable->importModule)
				Print(ctx.output, " from '%.*s'", FMT_ISTR(variable->importModule->name));

			PrintLine(ctx.output);
		}
	}

	PrintLine(ctx.output, "{");

	for(unsigned i = 0; i < lowFunction->blocks.size(); i++)
		PrintBlock(ctx, lowModule, lowFunction->blocks[i]);

	PrintLine(ctx.output, "}");
	PrintLine(ctx.output);
}

void PrintGraph(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredModule *lowModule)
{
	ctx.code = lowModule->vmModule->code;

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
		PrintFunction(ctx, lowModule, lowModule->functions[i]);

	ctx.output.Flush();
}

void DumpGraph(RegVmLoweredModule *lowModule)
{
	OutputContext outputCtx;

	char outputBuf[4096];
	outputCtx.outputBuf = outputBuf;
	outputCtx.outputBufSize = 4096;

	char tempBuf[4096];
	outputCtx.tempBuf = tempBuf;
	outputCtx.tempBufSize = 4096;

	outputCtx.stream = OutputContext::FileOpen("inst_graph_reg_low.txt");
	outputCtx.writeStream = OutputContext::FileWrite;

	InstructionRegVmLowerGraphContext instLowerGraphCtx(outputCtx);

	instLowerGraphCtx.showSource = true;

	PrintGraph(instLowerGraphCtx, lowModule);

	OutputContext::FileClose(outputCtx.stream);
	outputCtx.stream = NULL;
}
