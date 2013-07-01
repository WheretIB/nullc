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
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	#include "Executor_LLVM.h"
#endif

#include "StdLib.h"
#include "BinaryCache.h"

#include "includes/typeinfo.h"
#include "includes/dynamic.h"

CompilerError				CodeInfo::lastError;

FastVector<FunctionInfo*>	CodeInfo::funcInfo;
FastVector<VariableInfo*>	CodeInfo::varInfo;
FastVector<TypeInfo*>		CodeInfo::typeInfo;
HashMap<TypeInfo*>			CodeInfo::classMap;
FastVector<TypeInfo*>		CodeInfo::typeArrays;
FastVector<TypeInfo*>		CodeInfo::typeFunctions;
AliasInfo					*CodeInfo::globalAliases = NULL;

FastVector<NamespaceInfo*>	CodeInfo::namespaceInfo;

SourceInfo					CodeInfo::cmdInfoList;
FastVector<VMCmd>			CodeInfo::cmdList;
FastVector<NodeZeroOP*>		CodeInfo::nodeList;
FastVector<NodeZeroOP*>		CodeInfo::funcDefList;
const char*					CodeInfo::lastKnownStartPos = NULL;

Lexeme						*CodeInfo::lexStart = NULL, *CodeInfo::lexFullStart = NULL;

class Executor;
class ExecutorX86;
class ExecutorLLVM;

namespace NULLC
{
	Compiler*	compiler;

#ifndef NULLC_NO_EXECUTOR
	Linker*		linker;
#endif

	Executor*		executor;
	ExecutorX86*	executorX86;
	ExecutorLLVM*	executorLLVM;

	const char*	nullcLastError = NULL;

	unsigned int currExec = 0;
	char	*argBuf = NULL;

	bool initialized = false;
}

#define NULLC_CHECK_INITIALIZED(retval) if(!initialized){ nullcLastError = "ERROR: NULLC is not initialized"; return retval; }

void	nullcInit(const char* importPath)
{
	nullcInitCustomAlloc(NULL, NULL, importPath);
}

void	nullcInitCustomAlloc(void* (NCDECL *allocFunc)(int), void (NCDECL *deallocFunc)(void*), const char* importPath)
{
	using namespace NULLC;
	if(initialized)
	{
		nullcLastError = "ERROR: NULLC is already initialized";
		return;
	}
	nullcLastError = "";

	NULLC::alloc = allocFunc ? allocFunc : NULLC::defaultAlloc;
	NULLC::dealloc = deallocFunc ? deallocFunc : NULLC::defaultDealloc;
	NULLC::fileLoad = NULLC::defaultFileLoad;

	CodeInfo::funcInfo.reserve(256);
	CodeInfo::varInfo.reserve(256);
	CodeInfo::typeInfo.reserve(256);
	CodeInfo::typeArrays.reserve(64);
	CodeInfo::typeFunctions.reserve(128);
	CodeInfo::cmdList.reserve(2048);
	CodeInfo::nodeList.reserve(1024);
	CodeInfo::funcDefList.reserve(64);
	CodeInfo::namespaceInfo.reserve(64);
	CodeInfo::classMap.init();

	BinaryCache::Initialize();
	BinaryCache::SetImportPath(importPath);

	compiler = NULLC::construct<Compiler>();
#ifndef NULLC_NO_EXECUTOR
	linker = NULLC::construct<Linker>();
	executor = new(NULLC::alloc(sizeof(Executor))) Executor(linker);
	NULLC::SetGlobalLimit(NULLC_DEFAULT_GLOBAL_MEMORY_LIMIT);
#endif
#ifdef NULLC_BUILD_X86_JIT
	executorX86 = new(NULLC::alloc(sizeof(ExecutorX86))) ExecutorX86(linker);
	bool initx86 = executorX86->Initialize();
	assert(initx86);
	(void)initx86;
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	executorLLVM = new(NULLC::alloc(sizeof(ExecutorLLVM))) ExecutorLLVM(linker);
#endif

	argBuf = (char*)NULLC::alloc(64 * 1024);

#ifndef NULLC_NO_EXECUTOR
	nullcInitTypeinfoModuleLinkerOnly(linker);
#endif

	initialized = true;
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
	using namespace NULLC;

	currExec = id;
}

#ifdef NULLC_BUILD_X86_JIT
nullres	nullcSetJiTStack(void* start, void* end, unsigned int flagMemoryAllocated)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	if(!executorX86->SetStackPlacement(start, end, flagMemoryAllocated))
	{
		nullcLastError = executorX86->GetExecError();
		return 0;
	}
	return 1;
}
#endif

#ifndef NULLC_NO_EXECUTOR
void nullcSetGlobalMemoryLimit(unsigned int limit)
{
	NULLC::SetGlobalLimit(limit);
}
#endif

nullres	nullcBindModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	nullres good = compiler->AddModuleFunction(module, ptr, name, index);
	if(good == 0)
		nullcLastError = compiler->GetError();
	return good;
}

nullres nullcLoadModuleBySource(const char* module, const char* code)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

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
			pos[-1] = '/';
	strcat(path, ".nc");

	if(BinaryCache::GetBytecode(path))
	{
		nullcLastError = "ERROR: module already loaded";
		return false;
	}

	char *bytecode = NULL;
	nullcGetBytecode(&bytecode);
	BinaryCache::PutBytecode(path, bytecode, NULL, 0);
	return 1;
}

nullres nullcLoadModuleByBinary(const char* module, const char* binary)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

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
			pos[-1] = '/';
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
	BinaryCache::PutBytecode(path, binary, NULL, 0);
	return 1;
}

void nullcRemoveModule(const char* module)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED((void)false);

	BinaryCache::RemoveBytecode(module);
}

const char* nullcEnumerateModules(unsigned id)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(NULL);

	return BinaryCache::EnumerateModules(id);
}

nullres	nullcCompile(const char* code)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	nullcLastError = "";
	nullres good = compiler->Compile(code);
	if(good == 0)
		nullcLastError = compiler->GetError();
	return good;
}

unsigned int nullcGetBytecode(char **bytecode)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	unsigned int size = compiler->GetBytecode(bytecode);
	// Load it into cache
	BinaryCache::LastBytecode(*bytecode);
	return size;
}

unsigned int nullcGetBytecodeNoCache(char **bytecode)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	return compiler->GetBytecode(bytecode);
}

void	nullcSaveListing(const char *fileName)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED((void)0);

	compiler->SaveListing(fileName);
}

void	nullcTranslateToC(const char *fileName, const char *mainName)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED((void)0);

#ifdef NULLC_ENABLE_C_TRANSLATION
	compiler->TranslateToC(fileName, mainName);
#else
	(void)fileName;
	(void)mainName;
#endif
}

void nullcClean()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED((void)0);

#ifndef NULLC_NO_EXECUTOR
	linker->CleanCode();
	executor->ClearBreakpoints();

	#ifdef NULLC_BUILD_X86_JIT
	executorX86->ClearNative();
	#endif
#endif
}

nullres nullcLinkCode(const char *bytecode)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

#ifndef NULLC_NO_EXECUTOR
	if(!linker->LinkCode(bytecode))
	{
		nullcLastError = linker->GetLinkError();
		return false;
	}
	nullcLastError = linker->GetLinkError();
#else
	(void)bytecode;
	nullcLastError = "No executor available, compile library without NULLC_NO_EXECUTOR";
#endif
#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_VM)
		executor->UpdateInstructionPointer();
#endif
	if(currExec == NULLC_X86)
	{
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
	if(currExec == NULLC_LLVM)
	{
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
		bool res = executorLLVM->TranslateToNative();
		if(!res)
		{
			nullcLastError = executorLLVM->GetExecError();
		}
#else
		nullcLastError = "LLVM JIT isn't available";
		return false;
#endif
	}
#ifndef NULLC_NO_EXECUTOR
	return true;
#else
	return false;
#endif
}

nullres nullcBuild(const char* code)
{
	using namespace NULLC;

	if(!nullcCompile(code))
		return false;
	char *bytecode = NULL;
	nullcGetBytecode(&bytecode);
	nullcClean();
	if(!nullcLinkCode(bytecode))
	{
		delete[] bytecode;
		return false;
	}
	delete[] bytecode;
	return true;
}

nullres	nullcRun()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

#ifndef NULLC_NO_EXECUTOR
	return nullcRunFunction(NULL);
#else
	nullcLastError = "No executor available, compile library without NULLC_NO_EXECUTOR";
	return false;
#endif
}

#ifndef NULLC_NO_EXECUTOR
const char*	nullcGetArgumentVector(unsigned int functionID, uintptr_t extra, va_list args)
{
	using namespace NULLC;

	// Copy arguments in argument buffer
	ExternFuncInfo	&func = linker->exFunctions[functionID];
	char *argPos = argBuf;
	for(unsigned int i = 0; i < func.paramCount; i++)
	{
		ExternLocalInfo &lInfo = linker->exLocals[func.offsetToFirstLocal + i];
		ExternTypeInfo &tInfo = linker->exTypes[lInfo.type];
		switch(tInfo.type)
		{
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
			*(int*)argPos = va_arg(args, int);
			argPos += 4;
			break;
		case ExternTypeInfo::TYPE_LONG:
			*(long long*)argPos = va_arg(args, long long);
			argPos += 8;
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
			static unsigned int autoRefHash = GetStringHash("auto ref");
			static unsigned int autoArrayHash = GetStringHash("auto[]");
#ifdef _WIN64
			if(tInfo.size <= 4)
				*(int*)argPos = va_arg(args, int);
			else if(tInfo.size <= 8)
				*(long long*)argPos = va_arg(args, long long);
			else
				memcpy(argPos, va_arg(args, char*), tInfo.size);
			argPos += tInfo.size;
#elif defined(__x86_64__)
			if((tInfo.subCat == ExternTypeInfo::CAT_ARRAY && tInfo.arrSize == TypeInfo::UNSIZED_ARRAY) || tInfo.subCat == ExternTypeInfo::CAT_FUNCTION)
			{
				*(NULLCArray*)argPos = va_arg(args, NULLCArray);
				argPos += 12;
			}else if(tInfo.nameHash == autoRefHash){
				*(NULLCRef*)argPos = va_arg(args, NULLCRef);
				argPos += 12;
			}else if(tInfo.nameHash == autoArrayHash){
				*(NULLCAutoArray*)argPos = va_arg(args, NULLCAutoArray);
				argPos += sizeof(NULLCAutoArray);
			}else if(tInfo.subCat == ExternTypeInfo::CAT_CLASS){
				unsigned int *memberList = &linker->exTypeExtra[tInfo.memberOffset];
				for(unsigned int k = 0; k < tInfo.memberCount; k++)
				{
					const ExternTypeInfo &subType = linker->exTypes[memberList[k]];
					switch(subType.type)
					{
					case ExternTypeInfo::TYPE_CHAR:
					case ExternTypeInfo::TYPE_SHORT:
					case ExternTypeInfo::TYPE_INT:
						*(int*)argPos = va_arg(args, int);
						argPos += 4;
						break;
					case ExternTypeInfo::TYPE_LONG:
						*(long long*)argPos = va_arg(args, long long);
						argPos += 8;
						break;
					case ExternTypeInfo::TYPE_FLOAT:
						*(float*)argPos = (float)va_arg(args, double);
						argPos += 4;
						break;
					case ExternTypeInfo::TYPE_DOUBLE:
						*(double*)argPos = va_arg(args, double);
						argPos += 8;
						break;
					default:
						nullcLastError = "ERROR: unsupported complex function parameter";
						return NULL;
					}
				}
			}else{
				nullcLastError = "ERROR: unsupported complex function parameter";
				return NULL;
			}
#else
			for(unsigned int u = 0; u < tInfo.size >> 2; u++, argPos += 4)
				*(int*)argPos = va_arg(args, int);
#endif
			break;
		}
	}
	*(uintptr_t*)argPos = extra;
	argPos += sizeof(uintptr_t);

	return argBuf;
}
#endif

nullres	nullcRunFunction(const char* funcName, ...)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	nullres good = true;

#ifndef NULLC_NO_EXECUTOR
	static char	errorBuf[512];
	const char* argBuf = NULL;

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
		if(!argBuf)
			return false;
	}
#else
	(void)funcName;
#endif
	if(currExec == NULLC_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		executor->Run(functionID, argBuf);
		const char* error = executor->GetExecError();
		if(error[0] != '\0')
		{
			good = false;
			nullcLastError = error;
		}
#endif
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
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	}else if(currExec == NULLC_LLVM){
		executorLLVM->Run(functionID, argBuf);
		const char* error = executorLLVM->GetExecError();
		if(error[0] != '\0')
		{
			good = false;
			nullcLastError = error;
		}
#endif
	}else{
		good = false;
		nullcLastError = "Unknown executor code";
	}
	return good;
}

#ifndef NULLC_NO_EXECUTOR

void nullcThrowError(const char* error, ...)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED((void)0);

	va_list args;
	va_start(args, error);

	char buf[1024];

	if(error)
		vsnprintf(buf, 1024, error, args);
	else
		buf[0] = 0;
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
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	const char* error = NULL;
	// Copy arguments in argument buffer
	va_list args;
	va_start(args, ptr);
	const char *argBuf = nullcGetArgumentVector(ptr.id, (uintptr_t)ptr.context, args);
	if(!argBuf)
		return false;
	va_end(args);
	if(currExec == NULLC_VM)
	{
		executor->Run(ptr.id, argBuf);
		error = executor->GetExecError();
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Run(ptr.id, argBuf);
		error = executorX86->GetExecError();
#endif
	}else if(currExec == NULLC_LLVM){
#ifdef NULLC_LLVM_SUPPORT
		executorLLVM->Run(ptr.id, argBuf);
		error = executorLLVM->GetExecError();
#endif
	}
	if(error && error[0] != '\0')
	{
		nullcLastError = error;
		return false;
	}
	return true;
}

nullres nullcSetGlobal(const char* name, void* data)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	char* mem = (char*)nullcGetVariableData(NULL);
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
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	char* mem = (char*)nullcGetVariableData(NULL);
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

unsigned int nullcFindFunctionIndex(const char* name)
{
	using namespace NULLC;

	if(!linker)
	{
		nullcLastError = "ERROR: NULLC cannot find linked code";
		return ~0u;
	}
	if(!name)
	{
		nullcLastError = "ERROR: function name is 'null'";
		return ~0u;
	}
	unsigned int hash = GetStringHash(name);
	unsigned int index = ~0u;
	for(unsigned int i = 0; i < linker->exFunctions.size(); i++)
	{
		if(linker->exFunctions[i].nameHash == hash)
		{
			if(index != ~0u)
			{
				nullcLastError = "ERROR: there is more than one function with the same name";
				return ~0u;
			}
			index = i;
		}
	}
	if(index == ~0u)
	{
		nullcLastError = "ERROR: function with such name cannot be found";
		return ~0u;
	}
	if(linker->exFunctions[index].funcCat != ExternFuncInfo::NORMAL)
	{
		nullcLastError = "ERROR: function uses context, which is unavailable";
		return ~0u;
	}
	return index;
}

nullres nullcGetFunction(const char* name, NULLCFuncPtr* func)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	if(!func)
	{
		nullcLastError = "ERROR: passed pointer to NULLC function is 'null'";
		return false;
	}
	unsigned int index = nullcFindFunctionIndex(name);
	if(index == ~0u)
		return false;
	func->id = index;
	func->context = NULL;
	return true;
}

nullres nullcSetFunction(const char* name, NULLCFuncPtr func)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	unsigned int index = nullcFindFunctionIndex(name);
	if(index == ~0u)
		return false;
	if(linker->exFunctions[func.id].funcCat != ExternFuncInfo::NORMAL)
	{
		nullcLastError = "ERROR: source function uses context, which is unavailable";
		return false;
	}
	if(nullcGetCurrentExecutor(NULL) == NULLC_X86)
	{
		linker->UpdateFunctionPointer(index, func.id);
		if(linker->exFunctions[index].funcPtr && !linker->exFunctions[func.id].funcPtr)
		{
			nullcLastError = "Internal function cannot be overridden with external function on x86";
			return false;
		}
		if(linker->exFunctions[func.id].funcPtr && !linker->exFunctions[index].funcPtr)
		{
			nullcLastError = "External function cannot be overridden with internal function on x86";
			return false;
		}
	}
	linker->exFunctions[index].address = linker->exFunctions[func.id].address;
	linker->exFunctions[index].funcPtr = linker->exFunctions[func.id].funcPtr;
	linker->exFunctions[index].codeSize = linker->exFunctions[func.id].codeSize;
	return true;
}

nullres nullcIsStackPointer(void* ptr)
{
	NULLCRef r;
	r.ptr = (char*)ptr;
	return (nullres)IsPointerUnmanaged(r);
}

nullres nullcIsManagedPointer(void* ptr)
{
	return NULLC::IsBasePointer(ptr);
}

#endif

const char* nullcGetResult()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED("");

#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_VM)
		return executor->GetResult();
#endif
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResult();
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	if(currExec == NULLC_LLVM)
		return executorLLVM->GetResult();
#endif
	return "unknown executor";
}
int nullcGetResultInt()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_VM)
		return executor->GetResultInt();
#endif
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResultInt();
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	if(currExec == NULLC_LLVM)
		return executorLLVM->GetResultInt();
#endif
	return 0;
}
double nullcGetResultDouble()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0.0);

#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_VM)
		return executor->GetResultDouble();
#endif
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResultDouble();
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	if(currExec == NULLC_LLVM)
		return executorLLVM->GetResultDouble();
#endif
	return 0.0;
}
long long nullcGetResultLong()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_VM)
		return executor->GetResultLong();
#endif
#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		return executorX86->GetResultLong();
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	if(currExec == NULLC_LLVM)
		return executorLLVM->GetResultLong();
#endif
	return 0;
}

const char*	nullcGetLastError()
{
	return NULLC::nullcLastError;
}

#ifndef NULLC_NO_EXECUTOR
nullres nullcFinalize()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	FinalizeMemory();

	return 1;
}

void* nullcAllocate(unsigned int size)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);
	return NULLC::AllocObject(size);
}

void* nullcAllocateTyped(unsigned int typeID)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);
	return NULLC::AllocObject(nullcGetTypeSize(typeID), typeID);
}

NULLCArray nullcAllocateArrayTyped(unsigned int typeID, unsigned int count)
{
	using namespace NULLC;
	NULLCArray arr = { 0, 0 };
	NULLC_CHECK_INITIALIZED(arr);
	
	return NULLC::AllocArray(nullcGetTypeSize(typeID), count, typeID);
}

int nullcInitTypeinfoModule()
{
	return nullcInitTypeinfoModule(NULLC::linker);
}

int nullcInitDynamicModule()
{
	return nullcInitDynamicModule(NULLC::linker);
}
#endif

void nullcTerminate()
{
	using namespace NULLC;
	nullcLastError = "";
	if(!initialized)
		return;

	NULLC::dealloc(argBuf);
	argBuf = NULL;

	BinaryCache::Terminate();

	NULLC::destruct(compiler);
	compiler = NULL;
#ifndef NULLC_NO_EXECUTOR
	nullcDeinitTypeinfoModule();
	nullcDeinitDynamicModule();

	NULLC::destruct(linker);
	linker = NULL;
	NULLC::destruct(executor);
	executor = NULL;
#endif
#ifdef NULLC_BUILD_X86_JIT
	NULLC::destruct(executorX86);
	executorX86 = NULL;
#endif
#ifndef NULLC_NO_EXECUTOR
	NULLC::ResetMemory();
#endif
	CodeInfo::funcInfo.reset();
	CodeInfo::varInfo.reset();
	CodeInfo::typeInfo.reset();

	CodeInfo::cmdList.reset();
	CodeInfo::nodeList.reset();
	CodeInfo::funcDefList.reset();

	CodeInfo::cmdInfoList.Reset();

	CodeInfo::typeArrays.reset();
	CodeInfo::typeFunctions.reset();
	CodeInfo::classMap.reset();

	CodeInfo::namespaceInfo.reset();

	initialized = false;
}

//////////////////////////////////////////////////////////////////////////
/*						nullc_debug.h functions							*/

void* nullcGetVariableData(unsigned int *count)
{
	using namespace NULLC;

	if(currExec == NULLC_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		return executor->GetVariableData(count);
#endif
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		return executorX86->GetVariableData(count);
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	}else if(currExec == NULLC_LLVM){
		return executorLLVM->GetVariableData(count);
#endif
	}
	(void)count;
	return NULL;
}

unsigned int nullcGetCurrentExecutor(void **exec)
{
	using namespace NULLC;

#ifdef NULLC_BUILD_X86_JIT
	if(exec)
		*exec = (currExec == NULLC_VM ? (void*)executor : (currExec == NULLC_X86 ? (void*)executorX86 : (void*)executorLLVM));
#elif !defined(NULLC_NO_EXECUTOR)
	if(exec)
		*exec = executor;
#else
	*exec = NULL;
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

#ifndef NULLC_NO_EXECUTOR
ExternTypeInfo* nullcDebugTypeInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exTypes.size();
	return linker ? linker->exTypes.data : NULL;
}

ExternMemberInfo* nullcDebugTypeExtraInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exTypeExtra.size();
	return linker ? linker->exTypeExtra.data : NULL;
}

ExternVarInfo* nullcDebugVariableInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exVariables.size();
	return linker ? linker->exVariables.data : NULL;
}

ExternFuncInfo* nullcDebugFunctionInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exFunctions.size();
	return linker ? linker->exFunctions.data : NULL;
}

ExternLocalInfo* nullcDebugLocalInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exLocals.size();
	return linker ? linker->exLocals.data : NULL;
}

char* nullcDebugSymbols(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exSymbols.size();
	return linker ? linker->exSymbols.data : NULL;
}

char* nullcDebugSource()
{
	using namespace NULLC;

	return linker ? linker->exSource.data : NULL;
}

NULLCCodeInfo* nullcDebugCodeInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exCodeInfo.size() >> 1;
	return linker ? (NULLCCodeInfo*)linker->exCodeInfo.data : NULL;
}

VMCmd* nullcDebugCode(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exCode.size();
	return linker ? (VMCmd*)linker->exCode.data : NULL;
}

ExternModuleInfo* nullcDebugModuleInfo(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exModules.size();
	return linker ? linker->exModules.data : NULL;
}
#endif

void nullcDebugBeginCallStack()
{
	using namespace NULLC;

	if(currExec == NULLC_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		executor->BeginCallStack();
#endif
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		executorX86->BeginCallStack();
#endif
	}
}

unsigned int nullcDebugGetStackFrame()
{
	using namespace NULLC;

	unsigned int address = 0;
	// Get next address from call stack
	if(currExec == NULLC_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		address = executor->GetNextAddress();
#endif
	}else if(currExec == NULLC_X86){
#ifdef NULLC_BUILD_X86_JIT
		address = executorX86->GetNextAddress();
#endif
	}
	return address;
}

#ifndef NULLC_NO_EXECUTOR
nullres nullcDebugSetBreakFunction(unsigned (*callback)(unsigned int))
{
	using namespace NULLC;

	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executor->SetBreakFunction(callback);
#ifdef NULLC_BUILD_X86_JIT
	if(!executorX86)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executorX86->SetBreakFunction(callback);
#endif
	return true;
}

nullres nullcDebugClearBreakpoints()
{
	using namespace NULLC;

	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executor->ClearBreakpoints();
#ifdef NULLC_BUILD_X86_JIT
	if(!executorX86)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executorX86->ClearBreakpoints();
#endif
	return true;
}

nullres nullcDebugAddBreakpointImpl(unsigned int instruction, bool oneHit)
{
	using namespace NULLC;

	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	if(!executor->AddBreakpoint(instruction, oneHit))
	{
		nullcLastError = executor->GetExecError();
		return false;
	}
#ifdef NULLC_BUILD_X86_JIT
	if(!executorX86)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	if(!executorX86->AddBreakpoint(instruction, oneHit))
	{
		nullcLastError = executorX86->GetExecError();
		return false;
	}
#endif
	return true;
}

nullres nullcDebugAddBreakpoint(unsigned int instruction)
{
	return nullcDebugAddBreakpointImpl(instruction, false);
}

nullres nullcDebugAddOneHitBreakpoint(unsigned int instruction)
{
	return nullcDebugAddBreakpointImpl(instruction, true);
}

nullres nullcDebugRemoveBreakpoint(unsigned int instruction)
{
	using namespace NULLC;

	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	if(!executor->RemoveBreakpoint(instruction))
	{
		nullcLastError = executor->GetExecError();
		return false;
	}
#ifdef NULLC_BUILD_X86_JIT
	if(!executorX86)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	if(!executorX86->RemoveBreakpoint(instruction))
	{
		nullcLastError = executorX86->GetExecError();
		return false;
	}
#endif
	return true;
}

ExternFuncInfo* nullcDebugConvertAddressToFunction(int instruction, ExternFuncInfo* codeFunctions, unsigned functionCount)
{
	for(unsigned i = 0; i < functionCount; i++)
	{
		if(instruction > codeFunctions[i].address && instruction <= (codeFunctions[i].address + codeFunctions[i].codeSize))
			return&codeFunctions[i];
	}
	return NULL;
}

#endif

