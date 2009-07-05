#pragma once
#include "stdafx.h"
#include "Instruction_X86.h"

class Optimizer_x86
{
public:
	//std::vector<Command>*		HashListing(const char* pListing, int strSize);
	//std::vector<std::string>*	Optimize();
private:
	//void HashListing(const char*);
	void OptimizePushPop();
};