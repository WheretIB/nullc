#pragma once
#include "stdafx.h"
#include "Lexer.h"
#include "Bytecode.h"
#include "HashMap.h"

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
		if(!lineNum)
			return error;
		char *curr = errBuf;
		if(lineNum != 0)
			curr += sprintf(curr, "line %d - ", lineNum);
		curr += sprintf(curr, "%s", error);
		if(line[0] != 0)
		{
			curr += sprintf(curr, "\r\n  at \"%s\"", line);
			curr += sprintf(curr, "\r\n      ");
			for(unsigned int i = 0; i < shift; i++)
				*(curr++) = ' ';
			curr += sprintf(curr, "^\r\n");
		}
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

	bool	AddModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index);

	bool	Compile(const char* str, bool noClear = false);
	const char*		GetError();
	
	void	SaveListing(const char* fileName);
	void	TranslateToC(const char* fileName, const char *mainName);

	unsigned int	GetBytecode(char** bytecode);
private:
	void	ClearState();
	bool	ImportModule(const char* bytecode, const char* pos, unsigned int number);
	char*	BuildModule(const char* file, const char* altFile);

	Lexer	lexer;

	ChunkedStackPool<1020>			dupStrings;
	ChunkedStackPool<4092>			dupStringsModule;

	FastVector<TypeInfo*>			buildInTypes;
#ifdef NULLC_ENABLE_C_TRANSLATION
	FastVector<TypeInfo*>			translationTypes;
#endif

	FastVector<ExternModuleInfo>	activeModules;
	FastVector<char>				moduleSource;

	HashMap<TypeInfo*>				typeMap;
	FastVector<unsigned int>		typeRemap;

	unsigned int typeTop;
	unsigned int realGlobalCount;
};
