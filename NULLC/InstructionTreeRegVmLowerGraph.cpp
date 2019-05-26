#include "InstructionTreeRegVmLowerGraph.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"
#include "InstructionTreeRegVmLower.h"

void (*nullcDumpGraphRegVmLoweredModule)(RegVmLoweredModule*) = DumpGraph;

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

NULLC_PRINT_FORMAT_CHECK(2, 3) void Print(InstructionRegVmLowerGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);
}

void PrintIndent(InstructionRegVmLowerGraphContext &ctx)
{
	ctx.output.Print("  ");
}

void PrintLine(InstructionRegVmLowerGraphContext &ctx)
{
	ctx.output.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(2, 3) void PrintLine(InstructionRegVmLowerGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	PrintLine(ctx);
}

void PrintRegister(InstructionRegVmLowerGraphContext &ctx, unsigned char value)
{
	Print(ctx, "r%d", value);
}

void PrintFrameFlag(InstructionRegVmLowerGraphContext &ctx, unsigned char value)
{
	Print(ctx, value ? "relative" : "absolute");
}

void PrintConstant(InstructionRegVmLowerGraphContext &ctx, VmConstant *constant)
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

void PrintInstruction(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredInstruction *lowInstruction)
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
				Print(ctx, "// %.*s", unsigned(end - start), start);
				PrintLine(ctx);
				PrintIndent(ctx);

				if (lowInstruction->location->pos.end < end)
				{
					Print(ctx, "// ");

					for (unsigned i = 0; i < startOffset; i++)
					{
						Print(ctx, " ");

						if (start[i] == '\t')
							Print(ctx, i == 0 ? "  " : "   ");
					}

					for (unsigned i = startOffset; i < endOffset; i++)
					{
						Print(ctx, "~");

						if (start[i] == '\t')
							Print(ctx, i == 0 ? "~~" : "~~~");
					}

					PrintLine(ctx);
					PrintIndent(ctx);
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
				Print(ctx, "// %.*s", unsigned(end - start), start);
				PrintLine(ctx);
				PrintIndent(ctx);

				ctx.lastStart = start;
			}
		}
	}

	Print(ctx, "%s ", GetInstructionName(RegVmInstructionCode(lowInstruction->code)));

	bool skipComma = false;

	switch(lowInstruction->code)
	{
	case rviNop:
		break;
	case rviLoadByte:
	case rviLoadWord:
	case rviLoadDword:
	case rviLoadQword:
	case rviLoadFloat:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintFrameFlag(ctx, lowInstruction->rB);
		break;
	case rviLoadBytePtr:
	case rviLoadWordPtr:
	case rviLoadDwordPtr:
	case rviLoadQwordPtr:
	case rviLoadFloatPtr:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviLoadImm:
	case rviLoadImmHigh:
		PrintRegister(ctx, lowInstruction->rA);
		break;
	case rviStoreByte:
	case rviStoreWord:
	case rviStoreDword:
	case rviStoreQword:
	case rviStoreFloat:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintFrameFlag(ctx, lowInstruction->rB);
		break;
	case rviStoreBytePtr:
	case rviStoreWordPtr:
	case rviStoreDwordPtr:
	case rviStoreQwordPtr:
	case rviStoreFloatPtr:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviCombinedd:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rB);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviMov:
	case rviDtoi:
	case rviDtol:
	case rviDtof:
	case rviItod:
	case rviLtod:
	case rviItol:
	case rviLtoi:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviIndex:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rB);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviGetAddr:
		PrintFrameFlag(ctx, lowInstruction->rB);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviSetRange:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");

		switch(lowInstruction->rB)
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
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviJmp:
		skipComma = true;
		break;
	case rviJmpz:
	case rviJmpnz:
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviPush:
	case rviPushq:
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviCall:
		if(lowInstruction->rB != rvrVoid)
		{
			PrintRegister(ctx, lowInstruction->rA);
			Print(ctx, ", ");
		}
		switch(lowInstruction->rB)
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
		break;
	case rviCallPtr:
		if(lowInstruction->rB != rvrVoid)
		{
			PrintRegister(ctx, lowInstruction->rA);
			Print(ctx, ", ");
		}
		switch(lowInstruction->rB)
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
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviReturn:
		switch(lowInstruction->rB)
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
		break;
	case rviPushvtop:
		Print(ctx, "%d", lowInstruction->rB * 256 + lowInstruction->rC);
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
	case rviLogXor:
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
	case rviLogXorl:
	case rviAddd:
	case rviSubd:
	case rviMuld:
	case rviDivd:
	case rviPowd:
	case rviModd:
	case rviLessd:
	case rviGreaterd:
	case rviLequald:
	case rviGequald:
	case rviEquald:
	case rviNequald:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rB);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviNeg:
	case rviNegl:
	case rviNegd:
	case rviBitNot:
	case rviBitNotl:
	case rviLogNot:
	case rviLogNotl:
		PrintRegister(ctx, lowInstruction->rA);
		Print(ctx, ", ");
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviConvertPtr:
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviCheckRet:
		PrintRegister(ctx, lowInstruction->rC);
		break;
	case rviFuncAddr:
	case rviTypeid:
		PrintRegister(ctx, lowInstruction->rA);
		break;
	default:
		assert(!"unknown instruction");
	}

	if(lowInstruction->argument)
	{
		if(!skipComma)
			Print(ctx, ", ");

		PrintConstant(ctx, lowInstruction->argument);
	}

	PrintLine(ctx);
}

void PrintBlock(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredBlock *lowblock)
{
	PrintLine(ctx, "%.*s.b%d:", FMT_ISTR(lowblock->vmBlock->name), lowblock->vmBlock->uniqueId);

	for(RegVmLoweredInstruction *lowInstruction = lowblock->firstInstruction; lowInstruction; lowInstruction = lowInstruction->nextSibling)
	{
		PrintIndent(ctx);
		PrintInstruction(ctx, lowInstruction);
	}

	//if(lowblock->lastInstruction && !IsBlockTerminator(lowblock->lastInstruction))
	//	PrintLine(ctx, "  // fallthrough");

	PrintLine(ctx);
}

void PrintFunction(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredFunction *lowFunction)
{
	if(FunctionData *fData = lowFunction->vmFunction->function)
	{
		Print(ctx, "function %.*s.f%04x(", FMT_ISTR(fData->name->name), fData->uniqueId);

		for(unsigned i = 0; i < fData->arguments.size(); i++)
		{
			ArgumentData &argument = fData->arguments[i];

			Print(ctx, "%s%s%.*s %.*s", i == 0 ? "" : ", ", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name), FMT_ISTR(argument.name->name));
		}

		PrintLine(ctx, ")");

		PrintLine(ctx, "// argument size %lld", fData->argumentsSize);
	}
	else
	{
		PrintLine(ctx, "function global()");
	}

	if(ScopeData *scope = lowFunction->vmFunction->scope)
	{
		for(unsigned i = 0; i < scope->allVariables.size(); i++)
		{
			VariableData *variable = scope->allVariables[i];

			if(variable->isAlloca && variable->users.empty())
				continue;

			Print(ctx, "// %s0x%x: %.*s %.*s", variable->importModule ? "imported " : "", variable->offset, FMT_ISTR(variable->type->name), FMT_ISTR(variable->name->name));

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
				Print(ctx, " noalias");

			if(variable->isAlloca)
				Print(ctx, " alloca");

			if(variable->importModule)
				Print(ctx, " from '%.*s'", FMT_ISTR(variable->importModule->name));

			PrintLine(ctx);
		}
	}

	PrintLine(ctx, "{");

	for(unsigned i = 0; i < lowFunction->blocks.size(); i++)
		PrintBlock(ctx, lowFunction->blocks[i]);

	PrintLine(ctx, "}");
	PrintLine(ctx);
}

void PrintGraph(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredModule *lowModule)
{
	ctx.code = lowModule->vmModule->code;

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
		PrintFunction(ctx, lowModule->functions[i]);

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
