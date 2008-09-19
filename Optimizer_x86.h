#pragma once

class Optimizer_x86
{
public:
	HashListing(char*);
	Optimize(char*);
	OptimizePushPop(char*);
	bool IsRegister(char*);
	bool IsJump(int);
	char* Strnstr(char*, char*, int);
};