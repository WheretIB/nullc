#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Bytecode.h"

class Executor
{
public:
	Executor();
	~Executor();

	void	CleanCode();
	bool	LinkCode(const char *bytecode, int redefinitions);

	void	Run(const char* funcName = NULL) throw();

	const char*	GetResult() throw();
	const char*	GetExecError() throw();

	char*	GetVariableData();

	void	SetCallback(bool (*Func)(unsigned int));
private:
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	ofstream			m_FileStream;
#endif
	char		execError[512];
	char		execResult[64];

	FastVector<ExternTypeInfo*>	exTypes;
	FastVector<ExternVarInfo*>	exVariables;
	FastVector<ExternFuncInfo*>	exFunctions;
	FastVector<char>			exCode;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;

	struct ExternalFunctionInfo
	{
#if defined(_MSC_VER)
		unsigned int bytesToPop;
#elif defined(__CELLOS_LV2__)
		unsigned int rOffsets[8];
		unsigned int fOffsets[8];
#endif
	};
	FastVector<ExternalFunctionInfo>	exFuncInfo;
	bool CreateExternalInfo(ExternFuncInfo *fInfo, Executor::ExternalFunctionInfo& externalInfo);

	FastVector<asmStackType>	genStackTypes;

	FastVector<char, true>	genParams;
	FastVector<unsigned int>	paramTop;
	FastVector<char*>	fcallStack;

	unsigned int	*genStackBase;
	unsigned int	*genStackPtr;
	unsigned int	*genStackTop;

	OperFlag	retType;

	bool (*m_RunCallback)(unsigned int);
	
	bool RunExternalFunction(unsigned int funcID);
};

void PrintInstructionText(ostream* stream, CmdID cmd, unsigned int pos2, unsigned int valind, const CmdFlag cFlag, const OperFlag oFlag, unsigned int dw0=0, unsigned int dw1=0);
