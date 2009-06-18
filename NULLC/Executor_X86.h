#pragma once
#include "stdafx.h"

class ExecutorX86
{
public:
	ExecutorX86();
	~ExecutorX86();

	void	GenListing();
	string	GetListing();

	UINT	Run();
	string	GetResult();

	char*	GetVariableData();

	void	SetOptimization(bool toggle);
private:
	bool	optimize;

	ostringstream		logASM;

	char	*paramData;
	UINT	paramBase;

	char	*binCode;
	UINT	binCodeStart;
};