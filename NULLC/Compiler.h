#pragma once
#include "stdafx.h"

class CompilerError
{
public:
	CompilerError(const std::string& errStr, const char* apprPos);
	CompilerError(const char* errStr, const char* apprPos);
	~CompilerError(){}

	void Init(const char* errStr, const char* apprPos);

	template<class Ch, class Tr>

	friend basic_ostream<Ch, Tr>& operator<< (basic_ostream<Ch, Tr>& str, const CompilerError& err)
	{
		if(err.lineNum != 0)
			str << "line " << err.lineNum << " - ";
		str << err.error;
		if(err.line[0] != 0)
			str << "\r\n  at \"" << err.line << '\"';
		str << "\r\n      ";
		for(UINT i = 0; i < err.shift; i++)
			str << ' ';
		str << "^\r\n";
		return str;
	}

	static const char *codeStart;
private:
	char error[128];
	char line[128];
	UINT shift;
	UINT lineNum;
};

class Compiler
{
public:
	Compiler();
	~Compiler();

	bool	AddExternalFunction(void (NCDECL *ptr)(), const char* prototype);

	bool	Compile(string str);
	
	void	GenListing();
	string	GetListing();

	string	GetLog();

	unsigned int	GetBytecode(char **bytecode);
private:
	void	ClearState();

	ostringstream		logAST;
	ostringstream		logASM;
};