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

	FastVector<ExternTypeInfo*>	exTypes;
	FastVector<ExternVarInfo*>	exVariables;
	FastVector<ExternFuncInfo*>	exFunctions;
	FastVector<char>			exCode;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;

	struct ExternalFunctionInfo
	{
		unsigned int startInByteCode;
#if defined(_MSC_VER)
		unsigned int bytesToPop;
#elif defined(__CELLOS_LV2__)
		unsigned int rOffsets[8];
		unsigned int fOffsets[8];
#endif
	};
	FastVector<ExternalFunctionInfo>	exFuncInfo;
	bool CreateExternalInfo(ExternFuncInfo *fInfo, ExecutorX86::ExternalFunctionInfo& externalInfo);

	int	optimize;
	unsigned int		globalStartInBytecode;

	ostringstream		logASM;

	char	*paramData;
	unsigned int	paramBase;

	unsigned char	*binCode;
	unsigned int	binCodeStart;
	unsigned int	binCodeSize;
};