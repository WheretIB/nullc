#pragma once
#include "stdafx.h"
#include "ParseCommand.h"
#include "ParseClass.h"

class ExecutorX86
{
public:
	ExecutorX86(CommandList* cmds, std::vector<VariableInfo>* varinfo);
	~ExecutorX86();

	void	GenListing();
	string	GetListing();

	UINT	Run();
	string	GetResult();

	char*	GetVariableData();

	void	SetOptimization(bool toggle);
private:
	bool	optimize;

	CommandList	*cmdList;
	std::vector<VariableInfo>*	varInfo;

	CommandList	x86CmdList;

	ostringstream		logASM;

	char	*paramData;
	UINT	paramBase;
};