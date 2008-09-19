#pragma once

#ifdef VC6

#include <vector>
#include <string>

#endif

class Optimizer_x86
{
public:
	void HashListing(const char*);
	std::vector<std::string>* Optimize(const char*);
	void OptimizePushPop();
	bool IsRegister(const char*);
	bool IsJump(int);
	char* Strnstr(char*, char*, int);
};