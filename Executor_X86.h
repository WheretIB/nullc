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

	bool	Run();
	string	GetResult();
	string	GetVarInfo();

private:
	CommandList	*cmdList;
	std::vector<VariableInfo>*	varInfo;

	CommandList	x86CmdList;

	ostringstream		logASM;

	char	*paramData;
	UINT	paramBase;
};