#pragma once
#include "stdafx.h"

#include "ParseClass.h"

#include "Linker.h"

class ExecutorX86
{
public:
	ExecutorX86(Linker *linker);
	~ExecutorX86();

	void	GenListing();
	string	GetListing();

	void	Run(const char* funcName = NULL) throw();

	const char*	GetResult() throw();
	const char*	GetExecError() throw();

	char*	GetVariableData();

	void	SetOptimization(int toggle);
private:
	char	execError[256];
	char	execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo*>	&exTypes;
	FastVector<ExternFuncInfo*>	&exFunctions;
	FastVector<ExternalFunctionInfo>	&exFuncInfo;
	FastVector<char>			&exCode;

	int	optimize;
	unsigned int		globalStartInBytecode;

	ostringstream		logASM;

	char	*paramData;
	unsigned int	paramBase;

	unsigned char	*binCode;
	unsigned int	binCodeStart;
	unsigned int	binCodeSize;
};