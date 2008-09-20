#pragma once
#include "stdafx.h"

class Optimizer_x86
{
public:
	void HashListing(const char*);
	std::vector<std::string>* Optimize(const char* pListing, int strSize);
	void OptimizePushPop();
};