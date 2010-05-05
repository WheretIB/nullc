#include "stdafx.h"
#include "nullc.h"
#include "nullc_debug.h"

#include "CodeInfo.h"

#include "Compiler.h"
#include "Linker.h"
#include "Executor.h"
#ifdef NULLC_BUILD_X86_JIT
	#include "Executor_X86.h"
#endif

#include "StdLib.h"
#include "BinaryCache.h"

#include "includes/typeinfo.h"
#include "includes/dynamic.h"

CompilerError				CodeInfo::lastError;

FastVector<FunctionInfo*>	CodeInfo::funcInfo;
FastVector<VariableInfo*>	CodeInfo::varInfo;
FastVector<TypeInfo*>		CodeInfo::typeInfo;
FastVector<AliasInfo>		CodeInfo::aliasInfo;
HashMap<TypeInfo*>			CodeInfo::classMap;
FastVector<TypeInfo*>		CodeInfo::typeArrays;
FastVector<TypeInfo*>		CodeInfo::typeFunctions;

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
	BinaryCache::Initialize();
	BinaryCache::SetImportPath(importPath);
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

#ifdef NULLC_BUILD_X86_JIT
nullres	nullcSetJiTStack(void* start, void* end, unsigned int flagMemoryAllocated)
{
	if(!executorX86->SetStackPlacement(start, end, flagMemoryAllocated))
	{
		nullcLastError = executorX86->GetExecError();
		return 0;
	}
	return 1;
}
#endif

nullres	nullcBindModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index)
{
	nullres good = compiler->AddModuleFunction(module, ptr, name, index);
	if(good == 0)
		nullcLastError = compiler->GetError();
	return good;
}

nullres nullcLoadModuleBySource(const char* module, const char* code)
{
	if(!nullcCompile(code))
		return false;
	if(strlen(module) > 512)
	{
		nullcLastError = "ERROR: module name is too long";
		return false;
	}

	char	path[1024];
	strcpy(path, module);
	char	*pos = path;
	while(*pos)
		if(*pos++ == '.')
			pos[-1] = '\\';
	strcat(path, ".nc");

	if(BinaryCache::GetBytecode(path))
	{
		nullcLastError = "ERROR: module already loaded";
		return false;
	}

	char *bytecode = NULL;
	nullcGetBytecode(&bytecode);
	BinaryCache::PutBytecode(path, bytecode);
	return 1;
}

nullres nullcLoadModuleByBinary(const char* module, const char* binary)
{
	if(strlen(module) > 512)
	{
		nullcLastError = "ERROR: module name is too long";
		return false;
	}

	char	path[1024];
	strcpy(path, module);
	char	*pos = path;
	while(*pos)
		if(*pos++ == '.')
			pos[-1] = '\\';
	strcat(path, ".nc");

	if(BinaryCache::GetBytecode(path))
	{
		nullcLastError = "ERROR: module already loaded";
		return false;
	}
	// Duplicate binary
	char *copy = (char*)NULLC::alloc(((ByteCode*)binary)->size);
	memcpy(copy, binary, ((ByteCode*)binary)->size);
	binary = copy;
	// Load it into cache
	BinaryCache::PutBytecode(path, binary);
	return 1;
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
	unsigned int size = compiler->GetBytecode(bytecode);
	// Load it into cache
	BinaryCache::LastBytecode(*bytecode);
	return size;
}

unsigned int nullcGetBytecodeNoCache(char **bytecode)
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

void	nullcTranslateToC(const char *fileName, const char *mainName)
{
#ifdef NULLC_ENABLE_C_TRANSLATION
	compiler->TranslateToC(fileName, mainName);
#else
	(void)fileName;
	(void)mainName;
#endif
}

void nullcClean()
{
	linker->CleanCode();
	executor->ClearBreakpoints();
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
			nullcLastError = executorX86->GetExecError();
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

const char*	nullcGetArgumentVector(unsigned int functionID, unsigned int extra, va_list args)
{
	static char argBuf[64 * 1024];	// This is the maximum supported function argument size

	// Copy arguments in argument buffer
	ExternFuncInfo	&func = linker->exFunctions[functionID];
	char *argPos = argBuf;
	for(unsigned int i = 0; i < func.paramCount; i++)
	{
		ExternLocalInfo &lInfo = linker->exLocals[func.offsetToFirstLocal + i];
		switch(linker->exTypes[lInfo.type].type)
		{
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
			*(int*)argPos = va_arg(args, int);
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			*(float*)argPos = (float)va_arg(args, double);
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			*(double*)argPos = va_arg(args, double);
			argPos += 8;
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
			for(unsigned int u = 0; u < linker->exTypes[lInfo.type].size >> 2; u++, argPos += 4)
				*(int*)argPos = va_arg(args, int);
			break;
		}
	}
	*(int*)argPos = extra;
	argPos += 4;

	return argBuf;
}

nullres	nullcRunFunction(const char* funcName, ...)
{
	static char	errorBuf[512];
	const char* argBuf = NULL;

	nullres good = true;

	unsigned int functionID = ~0u;
	// If function is called, find it's index
	if(funcName)
	{
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)linker->exFunctions.size()-1; i >= 0; i--)
		{
			if(linker->exFunctions[i].nameHash == fnameHash)
			{
				functionID = i;
				break;
			}
		}
		if(functionID == ~0u)
		{
			SafeSprintf(errorBuf, 512, "ERROR: function %s not found", funcName);
			nullcLastError = errorBuf;
			return 0;
		}
		// Copy arguments in argument buffer
		va_list args;
		va_start(args, funcName);
		argBuf = nullcGetArgumentVector(functionID, 0, args);
		va_end(args);
	}

	if(currExec == NULLC_VM)
	{
		executor->Run(functionID, argBuf);
		const char* error = executor->GetExecError();
		if(error[0] != '\0')
		{
			good = false;
			nullcLastError = error;
		}
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Run(functionID, argBuf);
		const char* error = executorX86->GetExecError();
		if(error[0] != '\0')
		{
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

void nullcThrowError(const char* error, ...)
{
	va_list args;
	va_start(args, error);

	char buf[1024];

	vsnprintf(buf, 1024, error, args);
	buf[1024 - 1] = '\0';

	if(currExec == NULLC_VM)
	{
		executor->Stop(buf);
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Stop(buf);
#endif
	}
}

nullres		nullcCallFunction(NULLCFuncPtr ptr, ...)
{
	// Copy arguments in argument buffer
	va_list args;
	va_start(args, ptr);
	if(currExec == NULLC_VM)
	{
		executor->Run(ptr.id, nullcGetArgumentVector(ptr.id, (unsigned int)(uintptr_t)ptr.context, args));
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Run(ptr.id, nullcGetArgumentVector(ptr.id, (unsigned int)(uintptr_t)ptr.context, args));
#endif
	}
	va_end(args);
	const char* error = executor->GetExecError();
	if(error[0] != '\0')
	{
		nullcLastError = error;
		return 0;
	}
	return 1;
}

nullres nullcSetGlobal(const char* name, void* data)
{
	char* mem = (char*)nullcGetVariableData();
	if(!linker || !name || !data || !mem)
		return 0;
	unsigned int hash = GetStringHash(name);
	for(unsigned int i = 0; i < linker->exVariables.size(); i++)
	{
		if(linker->exVariables[i].nameHash == hash)
		{
			memcpy(mem + linker->exVariables[i].offset, data, linker->exTypes[linker->exVariables[i].type].size);
			return 1;
		}
	}
	return 0;
}

void* nullcGetGlobal(const char* name)
{
	char* mem = (char*)nullcGetVariableData();
	if(!linker || !name || !mem)
		return NULL;
	unsigned int hash = GetStringHash(name);
	for(unsigned int i = 0; i < linker->exVariables.size(); i++)
	{
		if(linker->exVariables[i].nameHash == hash)
			return mem + linker->exVariables[i].offset;
	}
	return NULL;
}


const char* nullcGetResult()
{
	if(currExec == NULLC_VM)
		return executor->GetResult();
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResult();
#endif
	return "unknown executor";
}
int nullcGetResultInt()
{
	if(currExec == NULLC_VM)
		return executor->GetResultInt();
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResultInt();
#endif
	return 0;
}
double nullcGetResultDouble()
{
	if(currExec == NULLC_VM)
		return executor->GetResultDouble();
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResultDouble();
#endif
	return 0.0;
}
long long nullcGetResultLong()
{
	if(currExec == NULLC_VM)
		return executor->GetResultLong();
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResultLong();
#endif
	return 0;
}

const char*	nullcGetLastError()
{
	return nullcLastError;
}

void* nullcAllocate(unsigned int size)
{
	return NULLC::AllocObject(size);
}

int nullcInitTypeinfoModule()
{
	return nullcInitTypeinfoModule(linker);
}

int nullcInitDynamicModule()
{
	return nullcInitDynamicModule(linker);
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

//////////////////////////////////////////////////////////////////////////
/*						nullc_debug.h functions							*/

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

unsigned int nullcGetCurrentExecutor(void **exec)
{
#ifdef NULLC_BUILD_X86_JIT
	if(exec)
		*exec = (currExec == NULLC_VM ? (void*)executor : (void*)executorX86);
#else
	if(exec)
		*exec = executor;
#endif
	return currExec;
}

const void* nullcGetModule(const char* path)
{
	char fullPath[256];
	SafeSprintf(fullPath, 256, "%s%s", BinaryCache::GetImportPath(), path);
	const char *bytecode = BinaryCache::GetBytecode(fullPath);
	if(!bytecode)
		bytecode = BinaryCache::GetBytecode(path);
	return bytecode;
}

ExternTypeInfo* nullcDebugTypeInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exTypes.size();
	return linker ? linker->exTypes.data : NULL;
}

unsigned int* nullcDebugTypeExtraInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exTypeExtra.size();
	return linker ? linker->exTypeExtra.data : NULL;
}

ExternVarInfo* nullcDebugVariableInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exVariables.size();
	return linker ? linker->exVariables.data : NULL;
}

ExternFuncInfo* nullcDebugFunctionInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exFunctions.size();
	return linker ? linker->exFunctions.data : NULL;
}

ExternLocalInfo* nullcDebugLocalInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exLocals.size();
	return linker ? linker->exLocals.data : NULL;
}

char* nullcDebugSymbols()
{
	return linker ? linker->exSymbols.data : NULL;
}

NULLCCodeInfo* nullcDebugCodeInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exCodeInfo.size() >> 1;
	return linker ? (NULLCCodeInfo*)linker->exCodeInfo.data : NULL;
}

ExternModuleInfo* nullcDebugModuleInfo(unsigned int *count)
{
	if(count && linker)
		*count = linker->exModules.size();
	return linker ? linker->exModules.data : NULL;
}

void nullcDebugBeginCallStack()
{
	if(currExec == NULLC_VM)
	{
		executor->BeginCallStack();
	}else{
#ifdef NULLC_BUILD_X86_JIT
		executorX86->BeginCallStack();
#endif
	}
}

unsigned int nullcDebugGetStackFrame()
{
	unsigned int address = 0;
	// Get next address from call stack
	if(currExec == NULLC_VM)
	{
		address = executor->GetNextAddress();
	}else{
#ifdef NULLC_BUILD_X86_JIT
		address = executorX86->GetNextAddress();
#endif
	}
	return address;
}

nullres nullcDebugSetBreakFunction(void (*callback)(unsigned int))
{
	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executor->SetBreakFunction(callback);
	return true;
}

nullres nullcDebugClearBreakpoints()
{
	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	if(currExec != NULLC_VM)
	{
		nullcLastError = "ERROR: breakpoints are supported only under VM";
		return false;
	}
	executor->ClearBreakpoints();
	return true;
}

nullres nullcDebugAddBreakpoint(unsigned int instruction)
{
	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	if(currExec != NULLC_VM)
	{
		nullcLastError = "ERROR: breakpoints are supported only under VM";
		return false;
	}
	if(!executor->AddBreakpoint(instruction))
	{
		nullcLastError = executor->GetExecError();
		return false;
	}
	return true;
}

