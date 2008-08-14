#include "stdafx.h"
#include "ParseFunc.h"

struct CompilerGrammar;

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
	CompilerGrammar*	m_data;
	CommandList*		m_cmds;
	ostringstream		m_astlog;
	ostringstream		m_asmlog;
};