#pragma once
#include "stdafx.h"

class ExecutorX86
{
public:
	ExecutorX86();
	~ExecutorX86();

	void	GenListing();
	string	GetListing();

	UINT	Run(const char* funcName = NULL);
	string	GetResult();

	char*	GetVariableData();

	void	SetOptimization(int toggle);
private:
	int	optimize;

	ostringstream		logASM;

	char	*paramData;
	UINT	paramBase;

	unsigned char	*binCode;
	UINT	binCodeStart;
	UINT	binCodeSize;
};