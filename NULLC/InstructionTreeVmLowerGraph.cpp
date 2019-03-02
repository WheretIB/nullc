#include "InstructionTreeVmLowerGraph.h"

#include "ExpressionTree.h"
#include "InstructionSet.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmLower.h"
#include "InstructionTreeVmCommon.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

NULLC_PRINT_FORMAT_CHECK(2, 3) void Print(InstructionVmLowerGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);
}

void PrintIndent(InstructionVmLowerGraphContext &ctx)
{
	ctx.output.Print("  ");
}

void PrintLine(InstructionVmLowerGraphContext &ctx)
{
	ctx.output.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(2, 3) void PrintLine(InstructionVmLowerGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	PrintLine(ctx);
}

void PrintConstant(InstructionVmLowerGraphContext &ctx, VmConstant *constant)
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
		Print(ctx, "%.*s.v%04x+0x%x%s", FMT_ISTR(constant->container->name), constant->container->uniqueId, constant->iValue, HasAddressTaken(constant->container) ? "" : " (noalias)");
	else if(constant->type.type == VM_TYPE_POINTER && constant->container)
		Print(ctx, "%.*s.v%04x%s", FMT_ISTR(constant->container->name), constant->container->uniqueId, HasAddressTaken(constant->container) ? "" : " (noalias)");
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

void PrintInstruction(InstructionVmLowerGraphContext &ctx, VmLoweredInstruction *lowInstruction)
{
	if(ctx.showSource && lowInstruction->location)
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

	if(lowInstruction->stackDepthBefore == lowInstruction->stackDepthAfter)
		Print(ctx, "[    %2d] ", lowInstruction->stackDepthAfter);
	else
		Print(ctx, "[%2d->%2d] ", lowInstruction->stackDepthBefore, lowInstruction->stackDepthAfter);

	Print(ctx, "%s", vmInstructionText[lowInstruction->cmd]);

	if(lowInstruction->flag)
	{
		Print(ctx, ", ");
		PrintConstant(ctx, lowInstruction->flag);
	}

	if(lowInstruction->helper)
	{
		Print(ctx, ", ");
		PrintConstant(ctx, lowInstruction->helper);
	}

	if(lowInstruction->argument)
	{
		Print(ctx, ", ");
		PrintConstant(ctx, lowInstruction->argument);
	}

	bool isTerminator = IsBlockTerminator(lowInstruction);
	bool isMemoryWrite = HasMemoryWrite(lowInstruction);
	bool isMemoryAccess = HasMemoryAccess(lowInstruction);

	if(isTerminator || isMemoryWrite || isMemoryAccess)
		Print(ctx, "\t//");

	if(isTerminator)
		Print(ctx, " block_term");

	if(isMemoryWrite)
		Print(ctx, " mem_write");
	else if(isMemoryAccess)
		Print(ctx, " mem_access");

	PrintLine(ctx);
}

void PrintBlock(InstructionVmLowerGraphContext &ctx, VmLoweredBlock *lowblock)
{
	PrintLine(ctx, "%.*s.b%d:", FMT_ISTR(lowblock->vmBlock->name), lowblock->vmBlock->uniqueId);

	for(VmLoweredInstruction *lowInstruction = lowblock->firstInstruction; lowInstruction; lowInstruction = lowInstruction->nextSibling)
	{
		PrintIndent(ctx);
		PrintInstruction(ctx, lowInstruction);
	}

	if(lowblock->lastInstruction && !IsBlockTerminator(lowblock->lastInstruction))
		PrintLine(ctx, "  // fallthrough");

	PrintLine(ctx);
}

void PrintFunction(InstructionVmLowerGraphContext &ctx, VmLoweredFunction *lowFunction)
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

			Print(ctx, "// %s0x%x: %.*s %.*s", variable->importModule ? "imported " : "", variable->offset, FMT_ISTR(variable->type->name), FMT_ISTR(variable->name));

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

void PrintGraph(InstructionVmLowerGraphContext &ctx, VmLoweredModule *lowModule)
{
	ctx.code = lowModule->vmModule->code;

	for(unsigned i = 0; i < lowModule->functions.size(); i++)
		PrintFunction(ctx, lowModule->functions[i]);

	Print(ctx, "// Removed register spill count: %d\n", lowModule->removedSpilledRegisters);

	ctx.output.Flush();
}

VariableData* FindGlobalAt(ExpressionContext &exprCtx, unsigned offset)
{
	unsigned targetModuleIndex = offset >> 24;

	if(targetModuleIndex)
		offset = offset & 0xffffff;

	for(unsigned i = 0; i < exprCtx.variables.size(); i++)
	{
		VariableData *variable = exprCtx.variables[i];

		unsigned variableModuleIndex = variable->importModule ? variable->importModule->importIndex : 0;

		if(IsGlobalScope(variable->scope) && variableModuleIndex == targetModuleIndex && offset >= variable->offset && (offset < variable->offset + variable->type->size || variable->type->size == 0))
			return variable;
	}

	return NULL;
}

void PrintInstructions(InstructionVmLowerGraphContext &ctx, InstructionVmFinalizeContext &lowerCtx, const char *code)
{
	assert(lowerCtx.locations.size() == lowerCtx.cmds.size());

	for(unsigned i = 0; i < lowerCtx.cmds.size(); i++)
	{
		SynBase *source = lowerCtx.locations[i];
		VMCmd &cmd = lowerCtx.cmds[i];

		if(ctx.showSource && source)
		{
			const char *start = source->pos.begin;
			const char *end = start + 1;

			// TODO: handle source locations from imported modules
			while(start > code && *(start - 1) != '\r' && *(start - 1) != '\n')
				start--;

			while(*end && *end != '\r' && *end != '\n')
				end++;

			if (ctx.showAnnotatedSource)
			{
				unsigned startOffset = unsigned(source->pos.begin - start);
				unsigned endOffset = unsigned(source->pos.end - start);

				if(start != ctx.lastStart || startOffset != ctx.lastStartOffset || endOffset != ctx.lastEndOffset)
				{
					Print(ctx, "%.*s\n", unsigned(end - start), start);

					if (source->pos.end < end)
					{
						for (unsigned k = 0; k < startOffset; k++)
						{
							Print(ctx, " ");

							if (start[k] == '\t')
								Print(ctx, "   ");
						}

						for (unsigned k = startOffset; k < endOffset; k++)
						{
							Print(ctx, "~");

							if (start[k] == '\t')
								Print(ctx, "~~~");
						}

						Print(ctx, "\n");
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
					Print(ctx, "%.*s\n", unsigned(end - start), start);

					ctx.lastStart = start;
				}
			}
		}

		char buf[256];
		cmd.Decode(buf);

		switch(cmd.cmd)
		{
		case cmdCall:
			if(FunctionData *function = lowerCtx.ctx.functions[cmd.argument])
				Print(ctx, "// %4d: %s (%.*s [%.*s]) param size %u\n", i, buf, FMT_ISTR(function->name->name), FMT_ISTR(function->type->name), (unsigned)function->argumentsSize);
			break;
		case cmdFuncAddr:
			if(FunctionData *function = lowerCtx.ctx.functions[cmd.argument])
				Print(ctx, "// %4d: %s (%.*s [%.*s])\n", i, buf, FMT_ISTR(function->name->name), FMT_ISTR(function->type->name));
			break;
		case cmdPushTypeID:
			Print(ctx, "// %4d: %s (%.*s)\n", i, buf, FMT_ISTR(lowerCtx.ctx.types[cmd.argument]->name));
			break;
		case cmdPushChar:
		case cmdPushShort:
		case cmdPushInt:
		case cmdPushFloat:
		case cmdPushDorL:
		case cmdPushCmplx:
		case cmdPushPtr:
		case cmdMovChar:
		case cmdMovShort:
		case cmdMovInt:
		case cmdMovFloat:
		case cmdMovDorL:
		case cmdMovCmplx:
		case cmdGetAddr:
			if(VariableData *global = cmd.flag == 0 ? FindGlobalAt(lowerCtx.ctx, cmd.argument) : NULL)
			{
				if(global->importModule)
				{
					if(global->offset == cmd.argument)
						Print(ctx, "// %4d: %s (%.*s [%.*s] from '%.*s')\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name), FMT_ISTR(global->importModule->name));
					else
						Print(ctx, "// %4d: %s (inside %.*s [%.*s] from '%.*s')\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name), FMT_ISTR(global->importModule->name));
				}
				else
				{
					if(global->offset == cmd.argument)
						Print(ctx, "// %4d: %s (%.*s [%.*s])\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name));
					else
						Print(ctx, "// %4d: %s (inside %.*s [%.*s])\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name));
				}
			}
			else
			{
				Print(ctx, "// %4d: %s\n", i, buf);
			}
			break;
		default:
			Print(ctx, "// %4d: %s\n", i, buf);
		}
	}

	ctx.output.Flush();
}

void DumpGraph(VmLoweredModule *lowModule)
{
	OutputContext outputCtx;

	char outputBuf[4096];
	outputCtx.outputBuf = outputBuf;
	outputCtx.outputBufSize = 4096;

	char tempBuf[4096];
	outputCtx.tempBuf = tempBuf;
	outputCtx.tempBufSize = 4096;

	outputCtx.stream = OutputContext::FileOpen("inst_graph_low.txt");
	outputCtx.writeStream = OutputContext::FileWrite;

	InstructionVmLowerGraphContext instLowerGraphCtx(outputCtx);

	instLowerGraphCtx.showSource = true;

	PrintGraph(instLowerGraphCtx, lowModule);

	OutputContext::FileClose(outputCtx.stream);
}
