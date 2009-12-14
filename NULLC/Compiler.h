#pragma once
#include "stdafx.h"
#include "Lexer.h"
#include "Bytecode.h"

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
	friend class Compiler;

	static const unsigned int ERROR_LENGTH = 256;

	char error[ERROR_LENGTH];
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
	bool	AddType(const char* typedecl);

	bool	Compile(const char* str, bool noClear = false);
	const char*		GetError();
	
	void	SaveListing(const char* fileName);

	unsigned int	GetBytecode(char** bytecode);
private:
	void	ClearState();
	bool	ImportModule(char* bytecode);

	Lexer	lexer;

	ChunkedStackPool<1020>	dupStrings;

	FastVector<ExternModuleInfo>	activeModules;

	unsigned int buildInFuncs;
	unsigned int basicTypes, buildInTypes;
	unsigned int typeTop, funcTop, varTop;
};
