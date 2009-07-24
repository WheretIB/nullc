#pragma once
#include "stdafx.h"
#include "Instruction_X86.h"

class OptimizerX86
{
public:
	unsigned int	Optimize(FastVector<x86Instruction, true, true>& instList);
private:
	unsigned int	SearchUp(unsigned int from);
	unsigned int	SearchDown(unsigned int from);
	unsigned int	OptimizationPass(FastVector<x86Instruction, true, true>& instList);

	x86Instruction	*start, *end;
};