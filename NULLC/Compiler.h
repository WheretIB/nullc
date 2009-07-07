#pragma once
#include "stdafx.h"

class CompilerError
{
public:
	CompilerError(const std::string& errStr, const char* apprPos);
	CompilerError(const char* errStr, const char* apprPos);
	~CompilerError(){}

	void Init(const char* errStr, const char* apprPos);

	const char* GetErrorString() const
	{
		static char errBuf[512];
		char *curr = errBuf;
		if(lineNum != 0)
			curr += sprintf(curr, "line %d - ", lineNum);
		curr += sprintf(curr, "%s", error);
		if(line[0] != 0)
			curr += sprintf(curr, "\r\n  at \"%s\"", line);
		curr += sprintf(curr, "\r\n      ");
		for(unsigned int i = 0; i < shift; i++)
			*(curr++) = ' ';
		curr += sprintf(curr, "^\r\n");
		return errBuf;
	}
	static const char *codeStart;
private:
	char error[128];
	char line[128];
	unsigned int shift;
	unsigned int lineNum;
};

class Compiler
{
public:
	Compiler();
	~Compiler();

	bool	AddExternalFunction(void (NCDECL *ptr)(), const char* prototype);

	bool	Compile(string str);
	
	void	SaveListing(const char *fileName);

	unsigned int	GetBytecode(char **bytecode);
private:
	void	ClearState();
};