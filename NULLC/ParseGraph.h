#pragma once

#include "Output.h"

struct SynBase;

struct ParseGraphContext
{
	ParseGraphContext(OutputContext &output): output(output)
	{
		depth = 0;
	}

	OutputContext &output;

	unsigned depth;

private:
	ParseGraphContext(const ParseGraphContext&);
	ParseGraphContext& operator=(const ParseGraphContext&);
};

void PrintGraph(ParseGraphContext &ctx, SynBase *syntax, const char *name);
