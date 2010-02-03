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

#include "includes/typeinfo.h"

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

const char* nulcExecuteResult = NULL;
const char*	nullcLastError = NULL;

unsigned int currExec = 0;

void	nullcInit(const char* importPath)
{
	nullcInitCustomAlloc(NULL, NULL, importPath);
}

void	nullcInitCustomAlloc(void* (NCDECL *allocFunc)(int), void (NCDECL *deallocFunc)(void*), const char* importPath)
{
	NULLC::alloc = allocFunc ? allocFunc : NULLC::defaultAlloc;
	NULLC::dealloc = deallocFunc ? deallocFunc : NULLC::defaultDealloc;
	NULLC::fileLoad = NULLC::defaultFileLoad;

	compiler = NULLC::construct<Compiler>();
	linker = NULLC::construct<Linker>();
	executor = new(NULLC::alloc(sizeof(Executor))) Executor(linker);
#ifdef NULLC_BUILD_X86_JIT
	executorX86 = new(NULLC::alloc(sizeof(ExecutorX86))) ExecutorX86(linker);
	executorX86->Initialize();
#endif
	BinaryCache::SetImportPath(importPath);

	bool res = nullcInitTypeinfoModule(linker);
	assert(res && "Failed to init typeinfo module");
}

void	nullcSetImportPath(const char* importPath)
{
	BinaryCache::SetImportPath(importPath);
}

void	nullcSetFileReadHandler(const void* (NCDECL *fileLoadFunc)(const char* name, unsigned int* size, int* nullcShouldFreePtr))
{
	NULLC::fileLoad = fileLoadFunc ? fileLoadFunc : NULLC::defaultFileLoad;
}

void	nullcSetExecutor(unsigned int id)
{
	currExec = id;
}

nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	nullres good = compiler->AddExternalFunction(ptr, prototype);
	if(good == 0)
		nullcLastError = compiler->GetError();
	return good;
}

nullres	nullcAddModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index)
{
	nullres good = compiler->AddModuleFunction(module, ptr, name, index);
	if(good == 0)
		nullcLastError = compiler->GetError();
	return good;
}

nullres	nullcCompile(const char* code)
{
	nullcLastError = "";
	nullres good = compiler->Compile(code);
	if(good == 0)
		nullcLastError = compiler->GetError();
	return good;
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
		nullcLastError = linker->GetLinkError();
		return false;
	}
	nullcLastError = linker->GetLinkError();
	if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		bool res = executorX86->TranslateToNative();
		if(!res)
		{
			nullcLastError = executor->GetResult();
		}
#else
		nullcLastError = "X86 JIT isn't available";
		return false;
#endif
	}
	return true;
}

nullres nullcBuild(const char* code)
{
	if(!nullcCompile(code))
		return false;
	char *bytecode = NULL;
	nullcGetBytecode(&bytecode);
	nullcClean();
	if(!nullcLinkCode(bytecode, 1))
		return false;
	delete[] bytecode;
	return true;
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
			nulcExecuteResult = executor->GetResult();
		}else{
			good = false;
			nullcLastError = error;
		}
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Run(funcName);
		const char* error = executorX86->GetExecError();
		if(error[0] == 0)
		{
			nulcExecuteResult = executorX86->GetResult();
		}else{
			good = false;
			nullcLastError = error;
		}
#else
		good = false;
		nullcLastError = "X86 JIT isn't available";
#endif
	}else{
		good = false;
		nullcLastError = "Unknown executor code";
	}
	return good;
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

const char* nullcGetResult()
{
	return nulcExecuteResult;
}

const char*	nullcGetLastError()
{
	return nullcLastError;
}

void* nullcGetVariableData()
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

void** nullcGetVariableInfo(unsigned int* count)
{
	*count = (unsigned int)CodeInfo::varInfo.size();
	return (void**)(&CodeInfo::varInfo[0]);
}

unsigned int nullcGetCurrentExecutor(void **exec)
{
#ifdef NULLC_BUILD_X86_JIT
	*exec = (currExec == NULLC_VM ? (void*)executor : (void*)executorX86);
#else
	*exec = executor;
#endif
	return currExec;
}

void* nullcGetModule(const char* path)
{
	char fullPath[256];
	SafeSprintf(fullPath, 256, "%s%s", BinaryCache::GetImportPath(), path);
	char *bytecode = BinaryCache::GetBytecode(fullPath);
	if(!bytecode)
		bytecode = BinaryCache::GetBytecode(path);
	return bytecode;
}

void nullcTerminate()
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
