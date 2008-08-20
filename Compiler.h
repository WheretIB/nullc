#include "stdafx.h"
#include "ParseFunc.h"

class Compiler
{
public:
	Compiler(CommandList* cmds);
	~Compiler();

	bool	Compile(string str);
	
	void	GenListing();
	string	GetListing();

	string	GetLog();

	std::vector<VariableInfo>*	GetVariableInfo();
private:
	CommandList*		cmdList;
	ostringstream		logAST;
	ostringstream		logASM;
};