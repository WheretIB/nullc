#pragma once
#include "stdafx.h"
#include "Lexer.h"

class CompilerError
{
public:
	CompilerError(){ shift = 0; lineNum = 0; empty = 1; }
	CompilerError(const char* errStr, const char* apprPos);
	~CompilerError(){}

	void Init(const char* errStr, const char* apprPos);
	unsigned int IsEmpty(){ return !!empty; }

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
	unsigned int empty;
};

class Compiler
{
public:
	Compiler();
	~Compiler();

	bool	AddExternalFunction(void (NCDECL *ptr)(), const char* prototype);

	bool	Compile(const char *str);
	const char*		GetError();
	
	void	SaveListing(const char *fileName);

	unsigned int	GetBytecode(char **bytecode);
private:
	void	ClearState();

	Lexer	lexer;
};