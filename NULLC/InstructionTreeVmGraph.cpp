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
	else if(type == VmType::Label)
		Print(ctx, "label");
	else if(type == VmType::Pointer)
		Print(ctx, "ptr");
	else if(type == VmType::FunctionRef)
		Print(ctx, "func_ref");
	else if(type == VmType::ArrayRef)
		Print(ctx, "arr_ref");
	else if(type == VmType::AutoRef)
		Print(ctx, "auto_ref");
	else if(type == VmType::AutoArray)
		Print(ctx, "auto_arr");
	else if(type.type == VM_TYPE_STRUCT)
		Print(ctx, "struct(%d)", type.size);
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
			Print(ctx, "%.*s.f%d", FMT_ISTR(fData->name), fData->uniqueId);
		else
			Print(ctx, "global");
	}
	else
	{
		assert(!"unknown type");
	}
}

void PrintUsers(InstructionVMGraphContext &ctx, VmValue *value, bool fullNames)
{
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
	else if(constant->type == VmType::Pointer)
		Print(ctx, "0x%x", constant->iValue);
	else if(constant->type.type == VM_TYPE_STRUCT)
		Print(ctx, "{ %d bytes }", constant->type.size);
	else
		assert(!"unknown type");
}

void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction)
{
	PrintUsers(ctx, instruction, false);

	if(instruction->type != VmType::Void)
	{
		PrintType(ctx, instruction->type);
		Print(ctx, " %%%d = ", instruction->uniqueId);
	}

	switch(instruction->cmd)
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
	case VM_INST_FRAME_OFFSET:
		Print(ctx, "frameoff");
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
	case VM_INST_LOG_AND:
		Print(ctx, "land");
		break;
	case VM_INST_LOG_OR:
		Print(ctx, "lor");
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
	}

	for(unsigned i = 0; i < instruction->arguments.size(); i++)
	{
		VmValue *value = instruction->arguments[i];

		if(value == instruction->arguments[0])
			Print(ctx, " ");
		else
			Print(ctx, ", ");

		PrintName(ctx, value, false);
	}

	if(instruction->type == VmType::Void)
		Print(ctx, " // %%%d", instruction->uniqueId);

	PrintLine(ctx);
}

void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block)
{
	PrintUsers(ctx, block, false);

	PrintLine(ctx, "%.*s.b%d:", FMT_ISTR(block->name), block->uniqueId);

	for(VmInstruction *value = block->firstInstruction; value; value = value->nextSibling)
	{
		Print(ctx, "  ");
		PrintInstruction(ctx, value);
	}

	PrintLine(ctx);
}

void PrintFunction(InstructionVMGraphContext &ctx, VmFunction *function)
{
	if(FunctionData *fData = function->function)
	{
		if(fData->isExternal && function->users.empty())
			return;

		PrintUsers(ctx, function, true);

		Print(ctx, "function ");
		PrintType(ctx, function->returnType);
		PrintLine(ctx, " %.*s.f%d()%s", FMT_ISTR(fData->name), fData->uniqueId, function->firstBlock == NULL ? ";" : "");

		if(function->firstBlock == NULL)
			return;
	}
	else
	{
		PrintLine(ctx, "function global()");
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
}
