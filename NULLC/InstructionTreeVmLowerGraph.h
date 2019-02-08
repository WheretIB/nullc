#pragma once

#include <stdio.h>

struct InstructionVMLowerContext;

struct InstructionVMLowerGraphContext
{
	InstructionVMLowerGraphContext(InstructionVMLowerContext &ctx): ctx(ctx)
	{
		file = 0;

		lastStart = 0;
		lastStartOffset = 0;
		lastEndOffset = 0;

		showSource = false;
		showAnnotatedSource = false;
	}

	InstructionVMLowerContext &ctx;

	FILE *file;

	const char *lastStart;
	unsigned lastStartOffset;
	unsigned lastEndOffset;

	bool showSource;
	bool showAnnotatedSource;

private:
	InstructionVMLowerGraphContext(const InstructionVMLowerGraphContext&);
	InstructionVMLowerGraphContext& operator=(const InstructionVMLowerGraphContext&);
};

void PrintInstructions(InstructionVMLowerGraphContext &ctx, const char *code);
