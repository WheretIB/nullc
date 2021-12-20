#include "InstructionTreeVmGraph.h"

#include <stdarg.h>

#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"
#include "TypeTree.h"

void (*nullcDumpGraphVmModule)(VmModule*) = DumpGraph;

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

namespace
{
	float DecodeFloat(int value)
	{
		float result;
		assert(sizeof(int) == sizeof(float));
		memcpy(&result, &value, sizeof(float));
		return result;
	}
}

NULLC_PRINT_FORMAT_CHECK(2, 3) void Print(InstructionVMGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);
}

void PrintIndent(InstructionVMGraphContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		ctx.output.Print("  ");
}

void PrintLine(InstructionVMGraphContext &ctx)
{
	ctx.output.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(2, 3) void PrintLine(InstructionVMGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	PrintLine(ctx);
}

void PrintType(InstructionVMGraphContext &ctx, VmType type)
{
	if(type == VmType::Void)
		Print(ctx, "void");
	else if(type == VmType::Int)
		Print(ctx, "int");
	else if(type == VmType::Double)
		Print(ctx, "double");
	else if(type == VmType::Long)
		Print(ctx, "long");
	else if(type == VmType::Block)
		Print(ctx, "label");
	else if(type == VmType::Function)
		Print(ctx, "function");
	else if(type.type == VM_TYPE_POINTER)
		Print(ctx, "ptr");
	else if(type.type == VM_TYPE_FUNCTION_REF)
		Print(ctx, "func_ref");
	else if(type.type == VM_TYPE_ARRAY_REF)
		Print(ctx, "array_ref");
	else if(type == VmType::AutoRef)
		Print(ctx, "auto ref");
	else if(type == VmType::AutoArray)
		Print(ctx, "auto[]");
	else if(type.type == VM_TYPE_STRUCT)
		Print(ctx, "%.*s", FMT_ISTR(type.structType->name));
	else
		assert(!"unknown type");
}

void PrintName(InstructionVMGraphContext &ctx, VmValue *value, bool fullName, bool noExtraInfo)
{
	if(VmConstant *constant = getType<VmConstant>(value))
	{
		PrintConstant(ctx, constant);
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		if(fullName)
		{
			if(VmBlock *block = inst->parent)
			{
				if(VmFunction *function = block->parent)
				{
					PrintName(ctx, function, true, true);
					Print(ctx, ".");
				}

				PrintName(ctx, block, true, true);
				Print(ctx, ".");
			}
		}

		Print(ctx, "%%%d", inst->uniqueId);

		if(inst->color)
			Print(ctx, ".c%d", inst->color);

		if(!inst->regVmRegisters.empty())
		{
			Print(ctx, ".r");

			for(unsigned i = 0; i < inst->regVmRegisters.size(); i++)
				Print(ctx, i == 0 ? "%d" : "|%d", inst->regVmRegisters[i]);
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		Print(ctx, "%.*s.b%d", FMT_ISTR(block->name), block->uniqueId);
	}
	else if(VmFunction *function = getType<VmFunction>(value))
	{
		if(FunctionData *fData = function->function)
			Print(ctx, "%.*s.f%04x", FMT_ISTR(fData->name->name), fData->uniqueId);
		else
			Print(ctx, "global");
	}
	else if(!value)
	{
		Print(ctx, "%%null%%");

		noExtraInfo = true;
	}
	else
	{
		assert(!"unknown type");
	}

	if(noExtraInfo)
		return;

	if(ctx.showFullTypes)
	{
		if(value->type.structType)
		{
			Print(ctx, " <%.*s>", FMT_ISTR(value->type.structType->name));
		}
		else
		{
			Print(ctx, " <");
			PrintType(ctx, value->type);
			Print(ctx, ">");
		}
	}

	if(ctx.showComments && !value->comment.empty())
		Print(ctx, " (%.*s)", FMT_ISTR(value->comment));
}

void PrintUsers(InstructionVMGraphContext &ctx, VmValue *value, bool fullNames)
{
	if(!ctx.showUsers)
		return;

	Print(ctx, "[");

	if(value->hasSideEffects)
		Print(ctx, "self");

	for(unsigned i = 0; i < value->users.size(); i++)
	{
		if(value->hasSideEffects || i != 0)
			Print(ctx, ", ");

		PrintName(ctx, value->users[i], fullNames, true);
	}

	Print(ctx, "] ");
}

void PrintConstant(InstructionVMGraphContext &ctx, VmConstant *constant)
{
	if(constant->type == VmType::Void)
		Print(ctx, "{}");
	else if(constant->type == VmType::Int && constant->isFloat)
		Print(ctx, "%ff", DecodeFloat(constant->iValue));
	else if(constant->type == VmType::Int)
		Print(ctx, "%d", constant->iValue);
	else if(constant->type == VmType::Double)
		Print(ctx, "%f", constant->dValue);
	else if(constant->type == VmType::Long)
		Print(ctx, "%lldl", constant->lValue);
	else if(constant->type.type == VM_TYPE_POINTER && constant->container && ctx.showContainers)
		Print(ctx, "%.*s+0x%x", FMT_ISTR(constant->container->name->name), constant->iValue);
	else if(constant->type.type == VM_TYPE_POINTER && constant->container)
		Print(ctx, "0x%x", constant->container->offset + constant->iValue);
	else if(constant->type.type == VM_TYPE_POINTER)
		Print(ctx, "0x%x", constant->iValue);
	else if(constant->type.type == VM_TYPE_STRUCT && constant->isReference && ctx.showContainers)
		Print(ctx, "*(%.*s+0x%x)", FMT_ISTR(constant->container->name->name), constant->iValue);
	else if(constant->type.type == VM_TYPE_STRUCT && constant->isReference)
		Print(ctx, "*(0x%x)", constant->container->offset + constant->iValue);
	else if(constant->type.type == VM_TYPE_STRUCT)
		Print(ctx, "{ %.*s }", FMT_ISTR(constant->type.structType->name));
	else if(constant->type.type == VM_TYPE_FUNCTION && constant->fValue->function)
		Print(ctx, "%.*s.f%04x", FMT_ISTR(constant->fValue->function->name->name), constant->fValue->function->uniqueId);
	else if(constant->type.type == VM_TYPE_FUNCTION)
		Print(ctx, "global.f0000");
	else
		assert(!"unknown type");
}

void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction)
{
	if(ctx.showSource && instruction->source && !instruction->source->isInternal)
	{
		const char *start = instruction->source->pos.begin;
		const char *end = start + 1;

		// TODO: handle source locations from imported modules
		while(start > ctx.code && *(start - 1) != '\r' && *(start - 1) != '\n')
			start--;

		while(*end && *end != '\r' && *end != '\n')
			end++;

		if (ctx.showAnnotatedSource)
		{
			unsigned startOffset = unsigned(instruction->source->pos.begin - start);
			unsigned endOffset = unsigned(instruction->source->pos.end - start);

			if(start != ctx.lastStart || startOffset != ctx.lastStartOffset || endOffset != ctx.lastEndOffset)
			{
				Print(ctx, "// %.*s", unsigned(end - start), start);
				PrintLine(ctx);
				PrintIndent(ctx);

				if (instruction->source->pos.end < end)
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

	PrintUsers(ctx, instruction, false);

	if(instruction->type != VmType::Void)
	{
		if(ctx.showTypes)
		{
			PrintType(ctx, instruction->type);
			Print(ctx, " ");
		}

		Print(ctx, "%%%d", instruction->uniqueId);

		if(instruction->color)
			Print(ctx, ".c%d", instruction->color);

		if(!instruction->regVmRegisters.empty())
		{
			Print(ctx, ".r");

			for(unsigned i = 0; i < instruction->regVmRegisters.size(); i++)
				Print(ctx, i == 0 ? "%d" : "|%d", instruction->regVmRegisters[i]);
		}

		if(ctx.showComments && !instruction->comment.empty())
			Print(ctx, " (%.*s)", FMT_ISTR(instruction->comment));

		if (ctx.showFullTypes)
		{
			if (instruction->type.structType)
			{
				Print(ctx, " <%.*s>", FMT_ISTR(instruction->type.structType->name));
			}
			else
			{
				Print(ctx, " <");
				PrintType(ctx, instruction->type);
				Print(ctx, ">");
			}
		}

		Print(ctx, " = ");
	}

	Print(ctx, "%s", GetInstructionName(instruction));

	if(instruction->cmd == VM_INST_PHI)
	{
		Print(ctx, " [");

		for(unsigned i = 0; i < instruction->arguments.size(); i += 2)
		{
			VmValue *value = instruction->arguments[i];
			VmValue *edge = instruction->arguments[i + 1];

			if(i != 0)
				Print(ctx, ", ");

			PrintName(ctx, value, false, false);
			Print(ctx, " from ");
			PrintName(ctx, edge, false, false);
		}

		Print(ctx, "]");
		PrintLine(ctx);

		return;
	}

	if(ctx.displayAsTree)
	{
		if(instruction->type == VmType::Void)
			Print(ctx, " // %%%d", instruction->uniqueId);

		PrintLine(ctx);

		ctx.depth++;

		for(unsigned i = 0; i < instruction->arguments.size(); i++)
		{
			VmValue *value = instruction->arguments[i];

			PrintIndent(ctx);

			VmInstruction *inst = getType<VmInstruction>(value);

			if(inst && !inst->users.empty())
			{
				PrintInstruction(ctx, inst);
			}
			else
			{
				PrintName(ctx, value, false, false);
				PrintLine(ctx);
			}
		}

		ctx.depth--;
	}
	else
	{
		for(unsigned i = 0; i < instruction->arguments.size(); i++)
		{
			VmValue *value = instruction->arguments[i];

			if(i == 0)
				Print(ctx, " ");
			else
				Print(ctx, ", ");

			PrintName(ctx, value, false, false);
		}

		if(instruction->type == VmType::Void)
			Print(ctx, " // %%%d", instruction->uniqueId);

		PrintLine(ctx);
	}
}

void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block)
{
	PrintUsers(ctx, block, false);

	PrintLine(ctx, "%.*s.b%d:", FMT_ISTR(block->name), block->uniqueId);

	ctx.depth++;

	Print(ctx, "  // predecessor blocks: [");

	for(unsigned i = 0; i < block->predecessors.size(); i++)
	{
		VmBlock *predecessor = block->predecessors[i];

		if(i != 0)
			Print(ctx, ", ");

		Print(ctx, "'%.*s.b%d'", FMT_ISTR(predecessor->name), predecessor->uniqueId);
	}

	PrintLine(ctx, "]");

	Print(ctx, "  // successor blocks: [");

	for(unsigned i = 0; i < block->successors.size(); i++)
	{
		VmBlock *successor = block->successors[i];

		if(i != 0)
			Print(ctx, ", ");

		Print(ctx, "'%.*s.b%d'", FMT_ISTR(successor->name), successor->uniqueId);
	}

	PrintLine(ctx, "]");

	Print(ctx, "  // immediate dominator: [");

	if(block->idom)
		Print(ctx, "'%.*s.b%d'", FMT_ISTR(block->idom->name), block->idom->uniqueId);

	PrintLine(ctx, "]");

	Print(ctx, "  // dominance frontier: [");

	for(unsigned i = 0; i < block->dominanceFrontier.size(); i++)
	{
		VmBlock *dominator = block->dominanceFrontier[i];

		if(i != 0)
			Print(ctx, ", ");

		Print(ctx, "'%.*s.b%d'", FMT_ISTR(dominator->name), dominator->uniqueId);
	}

	PrintLine(ctx, "]");

	Print(ctx, "  // dominance children: [");

	for(unsigned i = 0; i < block->dominanceChildren.size(); i++)
	{
		VmBlock *dominator = block->dominanceChildren[i];

		if(i != 0)
			Print(ctx, ", ");

		Print(ctx, "'%.*s.b%d'", FMT_ISTR(dominator->name), dominator->uniqueId);
	}

	PrintLine(ctx, "]");

	Print(ctx, "  // live variables going in: [");

	for(unsigned i = 0; i < block->liveIn.size(); i++)
	{
		VmInstruction *liveIn = block->liveIn[i];

		if(i != 0)
			Print(ctx, ", ");

		if(liveIn->cmd == VM_INST_PHI)
		{
			Print(ctx, "%%%d", liveIn->uniqueId);

			if(liveIn->color)
				Print(ctx, ".c%d", liveIn->color);

			if(!liveIn->regVmRegisters.empty())
			{
				Print(ctx, ".r");

				for(unsigned i = 0; i < liveIn->regVmRegisters.size(); i++)
					Print(ctx, i == 0 ? "%d" : "|%d", liveIn->regVmRegisters[i]);
			}

			if(liveIn->comment.empty())
				Print(ctx, " [");
			else
				Print(ctx, " (%.*s) [", FMT_ISTR(liveIn->comment));

			for(unsigned k = 0; k < liveIn->arguments.size(); k += 2)
			{
				VmInstruction *value = getType<VmInstruction>(liveIn->arguments[k]);

				if(k != 0)
					Print(ctx, ", ");

				if(value->comment.empty())
					Print(ctx, "%%%d", value->uniqueId);
				else
					Print(ctx, "%%%d (%.*s)", value->uniqueId, FMT_ISTR(value->comment));
			}

			Print(ctx, "]");
		}
		else
		{
			Print(ctx, "%%%d", liveIn->uniqueId);

			if(liveIn->color)
				Print(ctx, ".c%d", liveIn->color);

			if(!liveIn->regVmRegisters.empty())
			{
				Print(ctx, ".r");

				for(unsigned i = 0; i < liveIn->regVmRegisters.size(); i++)
					Print(ctx, i == 0 ? "%d" : "|%d", liveIn->regVmRegisters[i]);
			}

			if(!liveIn->comment.empty())
				Print(ctx, " (%.*s)", FMT_ISTR(liveIn->comment));
		}
	}

	PrintLine(ctx, "]");

	Print(ctx, "  // live variables going out: [");

	for(unsigned i = 0; i < block->liveOut.size(); i++)
	{
		VmInstruction *liveOut = block->liveOut[i];

		if(i != 0)
			Print(ctx, ", ");

		Print(ctx, "%%%d", liveOut->uniqueId);

		if(liveOut->color)
			Print(ctx, ".c%d", liveOut->color);

		if(!liveOut->regVmRegisters.empty())
		{
			Print(ctx, ".r");

			for(unsigned i = 0; i < liveOut->regVmRegisters.size(); i++)
				Print(ctx, i == 0 ? "%d" : "|%d", liveOut->regVmRegisters[i]);
		}

		if(!liveOut->comment.empty())
			Print(ctx, " (%.*s)", FMT_ISTR(liveOut->comment));
	}

	PrintLine(ctx, "]");

	for(VmInstruction *value = block->firstInstruction; value; value = value->nextSibling)
	{
		if(ctx.displayAsTree && !value->users.empty())
			continue;

		PrintIndent(ctx);
		PrintInstruction(ctx, value);
	}

	ctx.depth--;

	PrintLine(ctx);
}

void PrintFunction(InstructionVMGraphContext &ctx, VmFunction *function)
{
	if(FunctionData *fData = function->function)
	{
		if(fData->importModule != NULL && function->users.empty())
			return;

		PrintUsers(ctx, function, true);

		Print(ctx, "function ");
		PrintType(ctx, function->returnType);
		Print(ctx, " %.*s.f%04x(", FMT_ISTR(fData->name->name), fData->uniqueId);

		for(unsigned i = 0; i < fData->arguments.size(); i++)
		{
			ArgumentData &argument = fData->arguments[i];

			Print(ctx, "%s%s%.*s %.*s", i == 0 ? "" : ", ", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name), FMT_ISTR(argument.name->name));
		}

		Print(ctx, ")%s", function->firstBlock == NULL ? ";" : "");

		if(fData->importModule)
			Print(ctx, " from '%.*s'", FMT_ISTR(fData->importModule->name));

		PrintLine(ctx);

		if(function->firstBlock == NULL)
			return;
	}
	else
	{
		PrintLine(ctx, "function global()");
	}

	if(ScopeData *scope = function->scope)
	{
		for(unsigned i = 0; i < scope->allVariables.size(); i++)
		{
			VariableData *variable = scope->allVariables[i];

			if(variable->isAlloca && variable->users.empty())
				continue;

			Print(ctx, "// %s0x%x: %.*s %.*s", variable->importModule ? "imported " : "", variable->offset, FMT_ISTR(variable->type->name), FMT_ISTR(variable->name->name));

			if(ctx.showUsers)
				Print(ctx, " [");

			bool addressTaken = false;

			for(unsigned i = 0; i < variable->users.size(); i++)
			{
				VmConstant *user = variable->users[i];

				for(unsigned k = 0; k < user->users.size(); k++)
				{
					if(ctx.showUsers)
					{
						if(i != 0 || k != 0)
							Print(ctx, ", ");
					}

					if(VmInstruction *inst = getType<VmInstruction>(user->users[k]))
					{
						if(ctx.showUsers)
							PrintName(ctx, inst, inst->parent->parent != function, true);

						bool simpleUse = false;

						if(inst->cmd >= VM_INST_LOAD_BYTE && inst->cmd <= VM_INST_LOAD_STRUCT)
						{
							simpleUse = true;
						}
						else if(inst->cmd >= VM_INST_STORE_BYTE && inst->cmd <= VM_INST_STORE_STRUCT && inst->arguments[0] == user)
						{
							simpleUse = true;
						}
						else if(inst->cmd == VM_INST_MEM_COPY && (inst->arguments[0] == user || inst->arguments[2] == user))
						{
							simpleUse = true;
						}
						else if(inst->cmd == VM_INST_SET_RANGE && inst->arguments[0] == user)
						{
							simpleUse = true;
						}
						else if(inst->cmd == VM_INST_RETURN || inst->cmd == VM_INST_CALL)
						{
							if(user->isReference)
								simpleUse = true;
							else
								simpleUse = false;
						}
						else if(inst->cmd >= VM_INST_ADD_LOAD && inst->cmd <= VM_INST_BIT_XOR_LOAD && inst->arguments[1] == user)
						{
							simpleUse = true;
						}
						else
						{
							simpleUse = false;
						}

						if(!simpleUse)
							addressTaken = true;
					}
					else
					{
						assert(!"invalid constant use");
					}
				}
			}

			if(ctx.showUsers)
				Print(ctx, "]");

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

	for(VmBlock *value = function->firstBlock; value; value = value->nextSibling)
		PrintBlock(ctx, value);
	
	PrintLine(ctx, "}");
	PrintLine(ctx);
}

void PrintGraph(InstructionVMGraphContext &ctx, VmModule *module)
{
	ctx.code = module->code;

	for(VmFunction *value = module->functions.head; value; value = value->next)
		PrintFunction(ctx, value);

	PrintLine(ctx, "// Peephole optimizations: %d", module->peepholeOptimizations);
	PrintLine(ctx, "// Constant propagations: %d", module->constantPropagations);
	PrintLine(ctx, "// Dead code eliminations: %d", module->deadCodeEliminations);
	PrintLine(ctx, "// Control flow simplifications: %d", module->controlFlowSimplifications);
	PrintLine(ctx, "// Load store propagation: %d", module->loadStorePropagations);
	PrintLine(ctx, "// Common subexpression eliminations: %d", module->commonSubexprEliminations);
	PrintLine(ctx, "// Dead alloca store eliminations: %d", module->deadAllocaStoreEliminations);
	PrintLine(ctx, "// Function inlines: %d", module->functionInlines);

	ctx.output.Flush();
}

void DumpGraph(VmModule *module)
{
	OutputContext outputCtx;

	char outputBuf[4096];
	outputCtx.outputBuf = outputBuf;
	outputCtx.outputBufSize = 4096;

	char tempBuf[4096];
	outputCtx.tempBuf = tempBuf;
	outputCtx.tempBufSize = 4096;

	outputCtx.stream = OutputContext::FileOpen("inst_graph.txt");
	outputCtx.writeStream = OutputContext::FileWrite;

	InstructionVMGraphContext instGraphCtx(outputCtx);

	instGraphCtx.showUsers = true;
	instGraphCtx.displayAsTree = false;
	instGraphCtx.showFullTypes = true;
	instGraphCtx.showSource = true;

	PrintGraph(instGraphCtx, module);

	OutputContext::FileClose(outputCtx.stream);
	outputCtx.stream = NULL;
}
