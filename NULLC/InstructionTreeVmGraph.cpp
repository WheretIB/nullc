#include "InstructionTreeVmGraph.h"

#include <stdarg.h>

#include "InstructionTreeVm.h"
#include "TypeTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

void Print(InstructionVMGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);
}

void PrintIndent(InstructionVMGraphContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		fprintf(ctx.file, "  ");
}

void PrintLine(InstructionVMGraphContext &ctx)
{
	fprintf(ctx.file, "\n");
}

void PrintLine(InstructionVMGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

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

void PrintName(InstructionVMGraphContext &ctx, VmValue *value, bool fullName)
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
					PrintName(ctx, function, true);
					Print(ctx, ".");
				}

				PrintName(ctx, block, true);
				Print(ctx, ".");
			}
		}

		Print(ctx, "%%%d", inst->uniqueId);
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		Print(ctx, "%.*s.b%d", FMT_ISTR(block->name), block->uniqueId);
	}
	else if(VmFunction *function = getType<VmFunction>(value))
	{
		if(FunctionData *fData = function->function)
			Print(ctx, "%.*s.f%04x", FMT_ISTR(fData->name), fData->uniqueId);
		else
			Print(ctx, "global");
	}
	else
	{
		assert(!"unknown type");
	}

	if(!value->comment.empty())
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

		PrintName(ctx, value->users[i], fullNames);
	}

	Print(ctx, "] ");
}

void PrintConstant(InstructionVMGraphContext &ctx, VmConstant *constant)
{
	if(constant->type == VmType::Void)
		Print(ctx, "{}");
	else if(constant->type == VmType::Int)
		Print(ctx, "%d", constant->iValue);
	else if(constant->type == VmType::Double)
		Print(ctx, "%f", constant->dValue);
	else if(constant->type == VmType::Long)
		Print(ctx, "%lldl", constant->lValue);
	else if(constant->type.type == VM_TYPE_POINTER && constant->container)
		Print(ctx, "%.*s+0x%x", FMT_ISTR(constant->container->name), constant->iValue);
	else if(constant->type.type == VM_TYPE_POINTER)
		Print(ctx, "0x%x", constant->iValue);
	else if(constant->type.type == VM_TYPE_STRUCT)
		Print(ctx, "{ %.*s }", FMT_ISTR(constant->type.structType->name));
	else
		assert(!"unknown type");
}

void PrintInstructionName(InstructionVMGraphContext &ctx, VmInstructionType cmd)
{
	switch(cmd)
	{
	case VM_INST_LOAD_BYTE:
		Print(ctx, "loadb");
		break;
	case VM_INST_LOAD_SHORT:
		Print(ctx, "loadw");
		break;
	case VM_INST_LOAD_INT:
		Print(ctx, "load");
		break;
	case VM_INST_LOAD_FLOAT:
		Print(ctx, "loadf");
		break;
	case VM_INST_LOAD_DOUBLE:
		Print(ctx, "loadd");
		break;
	case VM_INST_LOAD_LONG:
		Print(ctx, "loadl");
		break;
	case VM_INST_LOAD_STRUCT:
		Print(ctx, "loads");
		break;
	case VM_INST_LOAD_IMMEDIATE:
		Print(ctx, "loadimm");
		break;
	case VM_INST_STORE_BYTE:
		Print(ctx, "storeb");
		break;
	case VM_INST_STORE_SHORT:
		Print(ctx, "storew");
		break;
	case VM_INST_STORE_INT:
		Print(ctx, "store");
		break;
	case VM_INST_STORE_FLOAT:
		Print(ctx, "storef");
		break;
	case VM_INST_STORE_DOUBLE:
		Print(ctx, "stored");
		break;
	case VM_INST_STORE_LONG:
		Print(ctx, "storel");
		break;
	case VM_INST_STORE_STRUCT:
		Print(ctx, "stores");
		break;
	case VM_INST_DOUBLE_TO_INT:
		Print(ctx, "dti");
		break;
	case VM_INST_DOUBLE_TO_LONG:
		Print(ctx, "dtl");
		break;
	case VM_INST_DOUBLE_TO_FLOAT:
		Print(ctx, "dtf");
		break;
	case VM_INST_INT_TO_DOUBLE:
		Print(ctx, "itd");
		break;
	case VM_INST_LONG_TO_DOUBLE:
		Print(ctx, "ltd");
		break;
	case VM_INST_INT_TO_LONG:
		Print(ctx, "itl");
		break;
	case VM_INST_LONG_TO_INT:
		Print(ctx, "lti");
		break;
	case VM_INST_INDEX:
		Print(ctx, "index");
		break;
	case VM_INST_INDEX_UNSIZED:
		Print(ctx, "indexu");
		break;
	case VM_INST_FUNCTION_ADDRESS:
		Print(ctx, "faddr");
		break;
	case VM_INST_TYPE_ID:
		Print(ctx, "typeid");
		break;
	case VM_INST_SET_RANGE:
		Print(ctx, "setrange");
		break;
	case VM_INST_JUMP:
		Print(ctx, "jmp");
		break;
	case VM_INST_JUMP_Z:
		Print(ctx, "jmpz");
		break;
	case VM_INST_JUMP_NZ:
		Print(ctx, "jmpnz");
		break;
	case VM_INST_CALL:
		Print(ctx, "call");
		break;
	case VM_INST_RETURN:
		Print(ctx, "ret");
		break;
	case VM_INST_YIELD:
		Print(ctx, "yield");
		break;
	case VM_INST_ADD:
		Print(ctx, "add");
		break;
	case VM_INST_SUB:
		Print(ctx, "sub");
		break;
	case VM_INST_MUL:
		Print(ctx, "mul");
		break;
	case VM_INST_DIV:
		Print(ctx, "div");
		break;
	case VM_INST_POW:
		Print(ctx, "pow");
		break;
	case VM_INST_MOD:
		Print(ctx, "mod");
		break;
	case VM_INST_LESS:
		Print(ctx, "lt");
		break;
	case VM_INST_GREATER:
		Print(ctx, "gt");
		break;
	case VM_INST_LESS_EQUAL:
		Print(ctx, "lte");
		break;
	case VM_INST_GREATER_EQUAL:
		Print(ctx, "gte");
		break;
	case VM_INST_EQUAL:
		Print(ctx, "eq");
		break;
	case VM_INST_NOT_EQUAL:
		Print(ctx, "neq");
		break;
	case VM_INST_SHL:
		Print(ctx, "shl");
		break;
	case VM_INST_SHR:
		Print(ctx, "shr");
		break;
	case VM_INST_BIT_AND:
		Print(ctx, "and");
		break;
	case VM_INST_BIT_OR:
		Print(ctx, "or");
		break;
	case VM_INST_BIT_XOR:
		Print(ctx, "xor");
		break;
	case VM_INST_LOG_XOR:
		Print(ctx, "lxor");
		break;
	case VM_INST_NEG:
		Print(ctx, "neg");
		break;
	case VM_INST_BIT_NOT:
		Print(ctx, "not");
		break;
	case VM_INST_LOG_NOT:
		Print(ctx, "lnot");
		break;
	case VM_INST_CREATE_CLOSURE:
		Print(ctx, "create_closure");
		break;
	case VM_INST_CLOSE_UPVALUES:
		Print(ctx, "close_upvalues");
		break;
	case VM_INST_CONVERT_POINTER:
		Print(ctx, "convert_pointer");
		break;
	case VM_INST_CHECKED_RETURN:
		Print(ctx, "checked_return");
		break;
	case VM_INST_CONSTRUCT:
		Print(ctx, "construct");
		break;
	case VM_INST_ARRAY:
		Print(ctx, "array");
		break;
	case VM_INST_EXTRACT:
		Print(ctx, "extract");
		break;
	case VM_INST_PHI:
		Print(ctx, "phi");
		break;
	default:
		assert(!"unknown instruction");
	}
}

void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction)
{
	PrintUsers(ctx, instruction, false);

	if(instruction->type != VmType::Void)
	{
		PrintType(ctx, instruction->type);

		if(!instruction->comment.empty())
			Print(ctx, " %%%d (%.*s) = ", instruction->uniqueId, FMT_ISTR(instruction->comment));
		else
			Print(ctx, " %%%d = ", instruction->uniqueId);
	}

	PrintInstructionName(ctx, instruction->cmd);

	if(instruction->cmd == VM_INST_PHI)
	{
		for(unsigned i = 0; i < instruction->arguments.size(); i += 2)
		{
			VmValue *value = instruction->arguments[i];
			VmValue *edge = instruction->arguments[i + 1];

			Print(ctx, value == instruction->arguments[0] ? " [" : ", ");

			PrintName(ctx, value, false);
			Print(ctx, " from ");
			PrintName(ctx, edge, false);
		}

		Print(ctx, "]");
		PrintLine(ctx);

		return;
	}

	if(ctx.displayAsTree)
	{
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
				PrintName(ctx, value, false);
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

			if(value == instruction->arguments[0])
				Print(ctx, " ");
			else
				Print(ctx, ", ");

			PrintName(ctx, value, false);
		}

		if(instruction->type == VmType::Void && ctx.showUsers)
			Print(ctx, " // %%%d", instruction->uniqueId);

		PrintLine(ctx);
	}
}

void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block)
{
	PrintUsers(ctx, block, false);

	PrintLine(ctx, "%.*s.b%d:", FMT_ISTR(block->name), block->uniqueId);

	ctx.depth++;

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
		if(fData->imported && function->users.empty())
			return;

		PrintUsers(ctx, function, true);

		Print(ctx, "function ");
		PrintType(ctx, function->returnType);
		Print(ctx, " %.*s.f%04x(", FMT_ISTR(fData->name), fData->uniqueId);

		for(unsigned i = 0; i < fData->arguments.size(); i++)
		{
			ArgumentData &argument = fData->arguments[i];

			Print(ctx, "%s%s%.*s %.*s", i == 0 ? "" : ", ", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name), FMT_ISTR(argument.name));
		}

		PrintLine(ctx, ")%s", function->firstBlock == NULL ? ";" : "");

		if(function->firstBlock == NULL)
			return;
	}
	else
	{
		PrintLine(ctx, "function global()");
	}

	if(ScopeData *scope = function->scope)
	{
		for(unsigned i = 0; i < scope->variables.size(); i++)
		{
			VariableData *variable = scope->variables[i];

			Print(ctx, "// %s0x%x: %.*s %.*s", variable->imported ? "imported " : "", variable->offset, FMT_ISTR(variable->type->name), FMT_ISTR(variable->name));

			if(ctx.showUsers)
			{
				Print(ctx, " [");

				bool addressTaken = false;

				for(unsigned i = 0; i < variable->users.size(); i++)
				{
					VmConstant *user = variable->users[i];

					for(unsigned k = 0; k < user->users.size(); k++)
					{
						if(i != 0 || k != 0)
							Print(ctx, ", ");

						if(VmInstruction *inst = getType<VmInstruction>(user->users[k]))
						{
							PrintName(ctx, inst, inst->parent->parent != function);

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

				Print(ctx, "]");

				if(!addressTaken)
					Print(ctx, " noalias");
			}

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
	for(VmFunction *value = module->functions.head; value; value = value->next)
		PrintFunction(ctx, value);

	PrintLine(ctx, "// Peephole optimizations: %d", module->peepholeOptimizations);
	PrintLine(ctx, "// Constant propagations: %d", module->constantPropagations);
	PrintLine(ctx, "// Dead code eliminations: %d", module->deadCodeEliminations);
	PrintLine(ctx, "// Control flow simplifications: %d", module->controlFlowSimplifications);
	PrintLine(ctx, "// Load store propagation: %d", module->loadStorePropagations);
	PrintLine(ctx, "// Common subexpression eliminations: %d", module->commonSubexprEliminations);
}

void DumpGraph(VmModule *module)
{
	InstructionVMGraphContext instGraphCtx;

	instGraphCtx.file = fopen("instruction_graph.txt", "w");
	instGraphCtx.showUsers = true;
	instGraphCtx.displayAsTree = false;

	PrintGraph(instGraphCtx, module);

	fclose(instGraphCtx.file);
}
