#include "stdafx.h"
#include "nullc.h"

#include "CodeInfo.h"

#include "Compiler.h"
#include "Linker.h"
#include "Executor.h"
#ifdef NULLC_BUILD_X86_JIT
	#include "Executor_X86.h"
#endif

#include "Bytecode.h"

unsigned int CodeInfo::activeExecutor = 0;

CompilerError				CodeInfo::lastError;

FastVector<FunctionInfo*>	CodeInfo::funcInfo;
FastVector<VariableInfo*>	CodeInfo::varInfo;
FastVector<TypeInfo*>		CodeInfo::typeInfo(64);
SourceInfo					CodeInfo::cmdInfoList;
FastVector<VMCmd>			CodeInfo::cmdList;
FastVector<NodeZeroOP*>		CodeInfo::nodeList(64);
FastVector<NodeZeroOP*>		CodeInfo::funcDefList(32);
const char*					CodeInfo::lastKnownStartPos = NULL;

Compiler*	compiler;
Linker*		linker;
Executor*	executor;
#ifdef NULLC_BUILD_X86_JIT
	ExecutorX86*	executorX86;
#endif

const char*	compileError;

const char* executeResult;
const char* executeLog;

char* compilationLog;

unsigned int currExec = 0;

void	nullcInit()
{
	compiler = new Compiler();
	linker = new Linker();
	executor = new Executor(linker);
#ifdef NULLC_BUILD_X86_JIT
	executorX86 = new ExecutorX86(linker);
	executorX86->Initialize();
#endif
#ifdef NULLC_LOG_FILES
	compilationLog = NULL;
#endif
}

void	nullcSetExecutor(unsigned int id)
{
	currExec = id;
	CodeInfo::activeExecutor = currExec;
}

nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	nullres good = compiler->AddExternalFunction(ptr, prototype);
	if(good == 0)
		compileError = compiler->GetError();
	return good;
}

nullres	nullcAddType(const char* typedecl)
{
	nullres good = compiler->AddType(typedecl);
	if(good == 0)
		compileError = compiler->GetError();
	return good;
}

nullres	nullcCompile(const char* code)
{
	compileError = "";
	nullres good = compiler->Compile(code);
	if(good == 0)
		compileError = compiler->GetError();
	return good;
}

const char*	nullcGetCompilationError()
{
	return compileError;
}

unsigned int nullcGetBytecode(char **bytecode)
{
	return compiler->GetBytecode(bytecode);
}

const char*	nullcGetCompilationLog()
{
#ifdef NULLC_LOG_FILES
	FILE *cLog = fopen("compilelog.txt", "rb");
	if(!cLog)
		return "";
	fseek(cLog, 0, SEEK_END);
	unsigned int size = ftell(cLog);
	fseek(cLog, 0, SEEK_SET);
	delete[] compilationLog;
	compilationLog = new char[size+1];
	fread(compilationLog, 1, size, cLog);
	compilationLog[size] = 0;
	fclose(cLog);
	return compilationLog;
#else
	return "";
#endif
}

void	nullcSaveListing(const char *fileName)
{
#ifdef NULLC_LOG_FILES
	compiler->SaveListing(fileName);
#else
	(void)fileName;
#endif
}

void nullcClean()
{
	linker->CleanCode();
}

nullres nullcLinkCode(const char *bytecode, int acceptRedefinitions)
{
	if(!linker->LinkCode(bytecode, acceptRedefinitions))
	{
		executeLog = linker->GetLinkError();
		return false;
	}
	executeLog = linker->GetLinkError();
	if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		bool res = executorX86->TranslateToNative();
		if(!res)
		{
			executeLog = executor->GetResult();
		}
#else
		executeLog = "X86 JIT isn't available";
		return false;
#endif
	}
	return true;
}

const char*	nullcGetLinkLog()
{
	return executeLog;
}

nullres	nullcRun()
{
	return nullcRunFunction(NULL);
}

nullres	nullcRunFunction(const char* funcName)
{
	nullres good = true;
	if(currExec == NULLC_VM)
	{
		executor->Run(funcName);
		const char* error = executor->GetExecError();
		if(error[0] == 0)
		{
			executeResult = executor->GetResult();
		}else{
			good = false;
			executeLog = error;
		}
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Run(funcName);
		const char* error = executorX86->GetExecError();
		if(error[0] == 0)
		{
			executeResult = executorX86->GetResult();
		}else{
			good = false;
			executeLog = error;
		}
#else
		good = false;
		executeLog = "X86 JIT isn't available";
#endif
	}else{
		good = false;
		executeLog = "Unknown executor code";
	}
	return good;
}
const char*	nullcGetRuntimeError()
{
	return executeLog;
}

const char*	nullcGetResult()
{
	return executeResult;
}

void*	nullcGetVariableData()
{
	if(currExec == NULLC_VM)
	{
		return executor->GetVariableData();
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		return executorX86->GetVariableData();
#endif
	}
	return NULL;
}

void**	nullcGetVariableInfo(unsigned int* count)
{
	*count = (unsigned int)CodeInfo::varInfo.size();
	return (void**)(&CodeInfo::varInfo[0]);
}

void	nullcDeinit()
{
	delete compiler;
	compiler = NULL;
	delete linker;
	linker = NULL;
	delete executor;
	executor = NULL;
#ifdef NULLC_BUILD_X86_JIT
	delete executorX86;
	executorX86 = NULL;
#endif
#ifdef NULLC_LOG_FILES
	delete[] compilationLog;
	compilationLog = NULL;
#endif
}
