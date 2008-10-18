#pragma once
#include "stdafx.h"

class Compiler
{
public:
	Compiler();
	~Compiler();

	bool	Compile(string str);
	
	void	GenListing();
	string	GetListing();

	string	GetLog();
private:
	ostringstream		logAST;
	ostringstream		logASM;
};