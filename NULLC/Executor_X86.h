#pragma once
#include "stdafx.h"

#include "ParseClass.h"

#include "Bytecode.h"

class ExecutorX86
{
public:
	ExecutorX86();
	~ExecutorX86();

	void	GenListing();
	string	GetListing();

	void	CleanCode();
	bool	LinkCode(const char *bytecode, int redefinitions);

	void	Run(const char* funcName = NULL) throw();

	const char*	GetResult() throw();
	const char*	GetExecError() throw();

	char*	GetVariableData();

	void	SetOptimization(int toggle);
private:
	char	execError[256];
	char	execResult[64];

	int	optimize;

	ostringstream		logASM;

	char		*bytecode;
	ByteCode	*codeInfo;

	char	*paramData;
	UINT	paramBase;

	unsigned char	*binCode;
	UINT	binCodeStart;
	UINT	binCodeSize;
};