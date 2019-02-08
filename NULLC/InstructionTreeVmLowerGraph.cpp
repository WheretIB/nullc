#include "InstructionTreeVmLowerGraph.h"

#include "ExpressionTree.h"
#include "InstructionSet.h"
#include "InstructionTreeVmLower.h"
#include "InstructionTreeVmCommon.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

VariableData* FindGlobalAt(InstructionVMLowerGraphContext &ctx, unsigned offset)
{
	unsigned targetModuleIndex = offset >> 24;

	if(targetModuleIndex)
		offset = offset & 0xffffff;

	for(unsigned i = 0; i < ctx.ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.ctx.variables[i];

		unsigned variableModuleIndex = variable->importModule ? variable->importModule->importIndex : 0;

		if(IsGlobalScope(variable->scope) && variableModuleIndex == targetModuleIndex && offset >= variable->offset && (offset < variable->offset + variable->type->size || variable->type->size == 0))
			return variable;
	}

	return NULL;
}

void PrintInstructions(InstructionVMLowerGraphContext &ctx, const char *code)
{
	assert(ctx.ctx.locations.size() == ctx.ctx.cmds.size());

	for(unsigned i = 0; i < ctx.ctx.cmds.size(); i++)
	{
		SynBase *source = ctx.ctx.locations[i];
		VMCmd &cmd = ctx.ctx.cmds[i];

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
					fprintf(ctx.file, "%.*s\n", unsigned(end - start), start);

					if (source->pos.end < end)
					{
						for (unsigned k = 0; k < startOffset; k++)
						{
							fprintf(ctx.file, " ");

							if (start[k] == '\t')
								fprintf(ctx.file, "   ");
						}

						for (unsigned k = startOffset; k < endOffset; k++)
						{
							fprintf(ctx.file, "~");

							if (start[k] == '\t')
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

		switch(cmd.cmd)
		{
		case cmdCall:
			if(FunctionData *function = ctx.ctx.ctx.functions[cmd.argument])
				fprintf(ctx.file, "// %4d: %s (%.*s [%.*s]) param size %u\n", i, buf, FMT_ISTR(function->name), FMT_ISTR(function->type->name), (unsigned)function->argumentsSize);
			break;
		case cmdFuncAddr:
			if(FunctionData *function = ctx.ctx.ctx.functions[cmd.argument])
				fprintf(ctx.file, "// %4d: %s (%.*s [%.*s])\n", i, buf, FMT_ISTR(function->name), FMT_ISTR(function->type->name));
			break;
		case cmdPushTypeID:
			fprintf(ctx.file, "// %4d: %s (%.*s)\n", i, buf, FMT_ISTR(ctx.ctx.ctx.types[cmd.argument]->name));
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
			if(VariableData *global = cmd.flag == 0 ? FindGlobalAt(ctx, cmd.argument) : NULL)
			{
				if(global->importModule)
				{
					if(global->offset == cmd.argument)
						fprintf(ctx.file, "// %4d: %s (%.*s [%.*s] from '%.*s')\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name), FMT_ISTR(global->importModule->name));
					else
						fprintf(ctx.file, "// %4d: %s (inside %.*s [%.*s] from '%.*s')\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name), FMT_ISTR(global->importModule->name));
				}
				else
				{
					if(global->offset == cmd.argument)
						fprintf(ctx.file, "// %4d: %s (%.*s [%.*s])\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name));
					else
						fprintf(ctx.file, "// %4d: %s (inside %.*s [%.*s])\n", i, buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name));
				}
			}
			else
			{
				fprintf(ctx.file, "// %4d: %s\n", i, buf);
			}
			break;
		default:
			fprintf(ctx.file, "// %4d: %s\n", i, buf);
		}
	}
}
