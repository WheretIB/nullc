#pragma once
#include "stdafx.h"
#include "Lexer.h"
#include "Bytecode.h"
#include "HashMap.h"

void ThrowError(const char* pos, const char* err, ...);

class TypeInfo;

class CompilerError
{
	// One buffer is for the error itself, the other is for a decorated version (line number, cursor)
	static char	*errLocal, *errGlobal;
public:
	CompilerError(){ shift = 0; lineNum = 0; empty = 1; }
	CompilerError(const char* errStr, const char* apprPos);
	~CompilerError(){}

	void Init(const char* errStr, const char* apprPos);
	unsigned int IsEmpty(){ return !!empty; }

	const char* GetErrorString() const
	{
		if(!lineNum)
			return errLocal;
		char *curr = errGlobal;
		if(lineNum != 0)
			curr += SafeSprintf(curr, NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal), "line %d - ", lineNum);
		curr += SafeSprintf(curr, NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal), "%s", errLocal);
		if(line[0] != 0)
		{
			curr += SafeSprintf(curr, NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal), "\r\n  at \"%s\"", line);
			curr += SafeSprintf(curr, NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal), "\r\n      ");
			if((unsigned)(NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal)) > shift)
			{
				for(unsigned int i = 0; i < shift; i++)
					*(curr++) = ' ';
			}
			curr += SafeSprintf(curr, NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal), "^");
		}
		curr += SafeSprintf(curr, NULLC_ERROR_BUFFER_SIZE - int(curr - errGlobal), "\r\n");
		return errGlobal;
	}
	static const char *codeStart, *codeEnd;
private:
	friend class Compiler;

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
	void	RecursiveLexify(const char* bytecode);

	friend class CompilerError;

	Lexer	lexer;

	ChunkedStackPool<1020>			dupStrings;
	ChunkedStackPool<4092>			dupStringsModule;

	FastVector<TypeInfo*>			buildInTypes;
#ifdef NULLC_ENABLE_C_TRANSLATION
	FastVector<TypeInfo*>			translationTypes;
#endif

	FastVector<unsigned>			importStack;

	FastVector<ExternModuleInfo>	activeModules;
	FastVector<char>				moduleSource;

	HashMap<TypeInfo*>				typeMap;
	FastVector<unsigned int>		typeRemap;

	HashMap<unsigned int>			funcMap;

	unsigned int typeTop;
	unsigned int realGlobalCount;
	unsigned int baseModuleSize;

public:
	struct CodeRange
	{
		CodeRange(): start(NULL), end(NULL){}
		CodeRange(const char* start, const char* end): start(start), end(end){}
		const char *start, *end;
	};
	FastVector<CodeRange>	codeSourceRange;

	struct ModuleInfo
	{
		ModuleInfo(){}
		ModuleInfo(const char* name, const char* data, unsigned stream, CodeRange range): name(name), data(data), stream(stream), range(range){}
		const char	*name;
		const char	*data;
		unsigned	stream;
		CodeRange	range;
	};
	FastVector<ModuleInfo>	moduleStack;
};
