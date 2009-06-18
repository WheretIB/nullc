#pragma once
#include "stdafx.h"

class CompilerError
{
public:
	CompilerError(std::string& errStr, const char* apprPos);
	CompilerError(const char* errStr, const char* apprPos);
	~CompilerError(){}

	void Init(const char* errStr, const char* apprPos);

	template<class Ch, class Tr>

	friend basic_ostream<Ch, Tr>& operator<< (basic_ostream<Ch, Tr>& str, const CompilerError& err)
	{
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
};

class Compiler
{
public:
	Compiler();
	~Compiler();

	bool	AddExternalFunction(void (_cdecl *ptr)(), const char* prototype);

	bool	Compile(string str);
	
	void	GenListing();
	string	GetListing();

	string	GetLog();
private:
	void	ClearState();

	ostringstream		logAST;
	ostringstream		logASM;
};