#pragma once
#include "stdafx.h"
#include "ParseCommand.h"
#include "ParseClass.h"

class ExecutorX86
{
public:
	ExecutorX86(CommandList* cmds, std::vector<VariableInfo>* varinfo);
	~ExecutorX86();

	bool	Compile();
	void	GenListing();
	string	GetListing();

	bool	Run();
	string	GetResult();

private:
	CommandList*		cmdList;
	ostringstream		logAST;
	ostringstream		logASM;
};