#pragma once

#include <stdio.h>

struct SynBase;

struct ParseGraphContext
{
	ParseGraphContext()
	{
		file = 0;

		depth = 0;
	}

	FILE *file;

	unsigned depth;
};

void PrintGraph(ParseGraphContext &ctx, SynBase *syntax, const char *name);
