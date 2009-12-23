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
#include "StdLib.h"
#include "BinaryCache.h"

CompilerError				CodeInfo::lastError;

FastVector<FunctionInfo*>	CodeInfo::funcInfo;
FastVector<VariableInfo*>	CodeInfo::varInfo;
FastVector<TypeInfo*>		CodeInfo::typeInfo;
FastVector<AliasInfo>		CodeInfo::aliasInfo;
unsigned int				CodeInfo::classCount = 0;

SourceInfo					CodeInfo::cmdInfoList;
FastVector<VMCmd>			CodeInfo::cmdList;
FastVector<NodeZeroOP*>		CodeInfo::nodeList;
FastVector<NodeZeroOP*>		CodeInfo::funcDefList;
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

unsigned int currExec = 0;

void	nullcInit()
{
	nullcInitCustomAlloc(NULL, NULL);
}

void	nullcInitCustomAlloc(void* (NCDECL *allocFunc)(size_t), void (NCDECL *deallocFunc)(void*))
{
	NULLC::alloc = allocFunc ? allocFunc : NULLC::defaultAlloc;
	NULLC::dealloc = deallocFunc ? deallocFunc : NULLC::defaultDealloc;

	compiler = NULLC::construct<Compiler>();
	linker = NULLC::construct<Linker>();
	executor = new(NULLC::alloc(sizeof(Executor))) Executor(linker);
#ifdef NULLC_BUILD_X86_JIT
	executorX86 = new(NULLC::alloc(sizeof(ExecutorX86))) ExecutorX86(linker);
	executorX86->Initialize();
#endif
}

void	nullcSetImportPath(const char* path)
{
	BinaryCache::SetImportPath(path);
}

void	nullcSetExecutor(unsigned int id)
{
	currExec = id;
}

nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	nullres good = compiler->AddExternalFunction(ptr, prototype);
	if(good == 0)
		compileError = compiler->GetError();
	return good;
}

nullres	nullcAddModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index)
{
	nullres good = compiler->AddModuleFunction(module, ptr, name, index);
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

void nullcThrowError(const char* error)
{
	if(currExec == NULLC_VM)
	{
		executor->Stop(error);
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Stop(error);
#endif
	}
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
	BinaryCache::Terminate();

	NULLC::destruct(compiler);
	compiler = NULL;
	NULLC::destruct(linker);
	linker = NULL;
	NULLC::destruct(executor);
	executor = NULL;
#ifdef NULLC_BUILD_X86_JIT
	NULLC::destruct(executorX86);
	executorX86 = NULL;
#endif
	NULLC::ResetMemory();

	CodeInfo::funcInfo.reset();
	CodeInfo::varInfo.reset();
	CodeInfo::typeInfo.reset();
	CodeInfo::aliasInfo.reset();

	CodeInfo::cmdList.reset();
	CodeInfo::nodeList.reset();
	CodeInfo::funcDefList.reset();

	CodeInfo::cmdInfoList.Reset();
}
