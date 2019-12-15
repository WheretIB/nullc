#include "stdafx.h"
#include "nullc.h"
#include "nullc_debug.h"

#include "Compiler.h"
#include "Linker.h"
#include "Executor.h"
#ifdef NULLC_BUILD_X86_JIT
	#include "Executor_X86.h"
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	#include "Executor_LLVM.h"
#endif
#include "Executor_RegVm.h"

#include "StdLib.h"
#include "BinaryCache.h"
#include "Trace.h"

#include "includes/typeinfo.h"
#include "includes/dynamic.h"

class Executor;
class ExecutorX86;
class ExecutorLLVM;
class ExecutorRegVm;

class Linker;

namespace NULLC
{
	Linker*		linker;

	Executor*		executor;
	ExecutorX86*	executorX86;
	ExecutorLLVM*	executorLLVM;
	ExecutorRegVm*	executorRegVm;

	const char*	nullcLastError = NULL;

	unsigned currExec = NULLC_REG_VM;
	char *argBuf = NULL;

	bool initialized = false;

	char *errorBuf = NULL;

	char *outputBuf = NULL;

	char *tempOutputBuf = NULL;

	ChunkedStackPool<65532> pool;
	GrowingAllocatorRef<ChunkedStackPool<65532>, 16384> allocator(pool);

	CompilerContext *compilerCtx = NULL;

	bool enableLogFiles = false;

	void* (*openStream)(const char* name) = OutputContext::FileOpen;
	void (*writeStream)(void *stream, const char *data, unsigned size) = OutputContext::FileWrite;
	void (*closeStream)(void* stream) = OutputContext::FileClose;

	int optimizationLevel = 2;

	TraceContext *traceContext = NULL;
}

unsigned nullcFindFunctionIndex(const char* name);

#define NULLC_CHECK_INITIALIZED(retval) if(!initialized){ nullcLastError = "ERROR: NULLC is not initialized"; return retval; }

nullres nullcInit()
{
	return nullcInitCustomAlloc(NULL, NULL);
}

nullres nullcInitCustomAlloc(void* (*allocFunc)(int), void (*deallocFunc)(void*))
{
	using namespace NULLC;

	nullcLastError = "";

	if(initialized)
	{
		nullcLastError = "ERROR: NULLC is already initialized";
		return 0;
	}

	NULLC::traceContext = TraceGetContext();

	TRACE_SCOPE("nullc", "nullcInitCustomAlloc");

	NULLC::alloc = allocFunc ? allocFunc : NULLC::defaultAlloc;
	NULLC::dealloc = deallocFunc ? deallocFunc : NULLC::defaultDealloc;
	NULLC::fileLoad = NULLC::defaultFileLoad;

	errorBuf = (char*)NULLC::alloc(NULLC_ERROR_BUFFER_SIZE);
	outputBuf = (char*)NULLC::alloc(NULLC_OUTPUT_BUFFER_SIZE);
	tempOutputBuf = (char*)NULLC::alloc(NULLC_TEMP_OUTPUT_BUFFER_SIZE);

	BinaryCache::Initialize();
	BinaryCache::AddImportPath("");

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

#ifndef NULLC_NO_EXECUTOR
	executorRegVm = new(NULLC::alloc(sizeof(ExecutorRegVm))) ExecutorRegVm(linker);
#endif

	argBuf = (char*)NULLC::alloc(64 * 1024);

#ifndef NULLC_NO_EXECUTOR
	nullcInitTypeinfoModuleLinkerOnly(linker);
#endif

	initialized = true;

	if(!BuildBaseModule(&allocator, NULLC::optimizationLevel))
	{
		allocator.Clear();

		nullcLastError = "ERROR: Failed to initialize base module";
		return 0;
	}

	allocator.Clear();

	return 1;
}

void nullcClearImportPaths()
{
	BinaryCache::ClearImportPaths();
	BinaryCache::AddImportPath("");
}

void nullcAddImportPath(const char* importPath)
{
	BinaryCache::AddImportPath(importPath);
}

void nullcSetFileReadHandler(const char* (*fileLoadFunc)(const char* name, unsigned* size), void (*fileFreeFunc)(const char* data))
{
	NULLC::fileLoad = fileLoadFunc ? fileLoadFunc : NULLC::defaultFileLoad;
	NULLC::fileFree = fileFreeFunc ? fileFreeFunc : NULLC::defaultFileFree;
}

void nullcSetExecutor(unsigned id)
{
	using namespace NULLC;

	currExec = id;
}

nullres nullcSetExecutorStackSize(unsigned bytes)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

#ifndef NULLC_NO_EXECUTOR
	if(!executor->SetStackSize(bytes))
		return 0;

#ifdef NULLC_BUILD_X86_JIT
	if(!executorX86->SetStackSize(bytes))
		return 0;
#endif

#if defined(NULLC_LLVM_SUPPORT)
	if(!executorLLVM->SetStackSize(bytes))
		return 0;
#endif
	if(!executorRegVm->SetStackSize(bytes))
		return 0;
#endif

	(void)bytes;
	return 1;
}

#ifndef NULLC_NO_EXECUTOR
void nullcSetGlobalMemoryLimit(unsigned limit)
{
	NULLC::SetGlobalLimit(limit);
}
#endif

void nullcSetEnableLogFiles(int enable, void* (*openStream)(const char* name), void (*writeStream)(void *stream, const char *data, unsigned size), void (*closeStream)(void* stream))
{
	NULLC::enableLogFiles = enable != 0;

	if(openStream || writeStream || closeStream)
	{
		NULLC::openStream = openStream;
		NULLC::writeStream = writeStream;
		NULLC::closeStream = closeStream;
	}
	else
	{
		NULLC::openStream = OutputContext::FileOpen;
		NULLC::writeStream = OutputContext::FileWrite;
		NULLC::closeStream = OutputContext::FileClose;
	}
}

void nullcSetOptimizationLevel(int level)
{
	if(level < 0)
		level = 0;

	if(level > 2)
		level = 2;

	NULLC::optimizationLevel = level;
}

void nullcSetEnableTimeTrace(int enable)
{
	NULLC::traceContext = NULLC::TraceGetContext();
	NULLC::TraceSetEnabled(enable != 0);
}

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)

nullres	nullcBindModuleFunction(const char* module, void (*ptr)(), const char* name, int index)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcBindModuleFunction");
	TRACE_LABEL(module);

	assert(!compilerCtx);

	const char *errorPos = NULL;

	if(!AddModuleFunction(&allocator, module, ptr, NULL, NULL, name, index, &errorPos, errorBuf, NULLC_ERROR_BUFFER_SIZE, NULLC::optimizationLevel))
	{
		allocator.Clear();

		nullcLastError = errorBuf;

		return false;
	}

	allocator.Clear();

	return true;
}

#endif

nullres nullcBindModuleFunctionWrapper(const char* module, void *func, void (*ptr)(void *func, char* retBuf, char* argBuf), const char* name, int index)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcBindModuleFunctionWrapper");
	TRACE_LABEL(module);

	assert(!compilerCtx);

	assert(func);

	const char *errorPos = NULL;

	if(!AddModuleFunction(&allocator, module, NULL, func, ptr, name, index, &errorPos, errorBuf, NULLC_ERROR_BUFFER_SIZE, NULLC::optimizationLevel))
	{
		allocator.Clear();

		nullcLastError = errorBuf;

		return false;
	}

	allocator.Clear();

	return true;
}

nullres nullcLoadModuleBySource(const char* module, const char* code)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcLoadModuleBySource");
	TRACE_LABEL(module);

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
	nullcClean();

	BinaryCache::PutBytecode(path, bytecode, NULL, 0);
	return 1;
}

nullres nullcLoadModuleByBinary(const char* module, const char* binary)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcLoadModuleByBinary");
	TRACE_LABEL(module);

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

	TRACE_SCOPE("nullc", "nullcRemoveModule");
	TRACE_LABEL(module);

	BinaryCache::RemoveBytecode(module);
}

const char* nullcEnumerateModules(unsigned id)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(NULL);

	return BinaryCache::EnumerateModules(id);
}

nullres nullcAnalyze(const char* code)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcAnalyze");

	nullcLastError = "";

	NULLC::destruct(compilerCtx);

	allocator.Clear();

	compilerCtx = new(NULLC::alloc(sizeof(CompilerContext))) CompilerContext(&allocator, optimizationLevel, ArrayView<InplaceStr>());

	*errorBuf = 0;

	compilerCtx->errorBuf = errorBuf;
	compilerCtx->errorBufSize = NULLC_ERROR_BUFFER_SIZE;

	compilerCtx->enableLogFiles = enableLogFiles;

	compilerCtx->outputCtx.openStream = openStream;
	compilerCtx->outputCtx.writeStream = writeStream;
	compilerCtx->outputCtx.closeStream = closeStream;

	compilerCtx->outputCtx.outputBuf = outputBuf;
	compilerCtx->outputCtx.outputBufSize = NULLC_OUTPUT_BUFFER_SIZE;

	compilerCtx->outputCtx.tempBuf = tempOutputBuf;
	compilerCtx->outputCtx.tempBufSize = NULLC_TEMP_OUTPUT_BUFFER_SIZE;

	if(!AnalyzeModuleFromSource(*compilerCtx, code))
	{
		if(compilerCtx->errorPos)
			nullcLastError = compilerCtx->errorBuf;
		else
			nullcLastError = "ERROR: internal error";

		return 0;
	}

	return 1;
}

nullres	nullcCompile(const char* code)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	NULLC::TraceDump();

	TRACE_SCOPE("nullc", "nullcCompile");

	nullcLastError = "";

	NULLC::destruct(compilerCtx);

	allocator.Clear();

	compilerCtx = new(NULLC::alloc(sizeof(CompilerContext))) CompilerContext(&allocator, optimizationLevel, ArrayView<InplaceStr>());

	*errorBuf = 0;

	compilerCtx->errorBuf = errorBuf;
	compilerCtx->errorBufSize = NULLC_ERROR_BUFFER_SIZE;

	compilerCtx->enableLogFiles = enableLogFiles;

	compilerCtx->outputCtx.openStream = openStream;
	compilerCtx->outputCtx.writeStream = writeStream;
	compilerCtx->outputCtx.closeStream = closeStream;

	compilerCtx->outputCtx.outputBuf = outputBuf;
	compilerCtx->outputCtx.outputBufSize = NULLC_OUTPUT_BUFFER_SIZE;

	compilerCtx->outputCtx.tempBuf = tempOutputBuf;
	compilerCtx->outputCtx.tempBufSize = NULLC_TEMP_OUTPUT_BUFFER_SIZE;

	if(!CompileModuleFromSource(*compilerCtx, code))
	{
		if(compilerCtx->errorPos)
			nullcLastError = compilerCtx->errorBuf;
		else
			nullcLastError = "ERROR: internal error";

		return 0;
	}

	return 1;
}

unsigned nullcGetBytecode(char **bytecode)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	TRACE_SCOPE("nullc", "nullcGetBytecode");

	if(!compilerCtx)
	{
		nullcLastError = "ERROR: there is no active compiler context";
		return 0;
	}

	unsigned size = GetBytecode(*compilerCtx, bytecode);

	// Load it into cache
	BinaryCache::LastBytecode(*bytecode);
	return size;
}

unsigned nullcGetBytecodeNoCache(char **bytecode)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	TRACE_SCOPE("nullc", "nullcGetBytecodeNoCache");

	if(!compilerCtx)
	{
		nullcLastError = "ERROR: there is no active compiler context";
		return 0;
	}

	return GetBytecode(*compilerCtx, bytecode);
}

nullres nullcSaveListing(const char *fileName)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	TRACE_SCOPE("nullc", "nullcSaveListing");

	if(!compilerCtx)
	{
		nullcLastError = "ERROR: there is no active compiler context";
		return 0;
	}

	if(!SaveListing(*compilerCtx, fileName))
	{
		nullcLastError = compilerCtx->errorBuf;
		return 0;
	}

	return 1;
}

nullres	nullcTranslateToC(const char *fileName, const char *mainName, void (*addDependency)(const char *fileName))
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	TRACE_SCOPE("nullc", "nullcTranslateToC");

	if(!compilerCtx)
	{
		nullcLastError = "ERROR: there is no active compiler context";
		return 0;
	}

	if(!TranslateToC(*compilerCtx, fileName, mainName, addDependency))
	{
		nullcLastError = compilerCtx->errorBuf;
		return 0;
	}

	return 1;
}

void nullcClean()
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED((void)0);

	TRACE_SCOPE("nullc", "nullcClean");

#ifndef NULLC_NO_EXECUTOR
	linker->CleanCode();
	executor->ClearBreakpoints();

	#ifdef NULLC_BUILD_X86_JIT
	executorX86->ClearNative();
	#endif

	executorRegVm->ClearBreakpoints();
#endif

	nullcLastError = "";

	NULLC::destruct(compilerCtx);
	compilerCtx = NULL;

	allocator.Clear();
}

nullres nullcLinkCode(const char *bytecode)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcLinkCode");

#ifndef NULLC_NO_EXECUTOR
	if(!linker->LinkCode(bytecode, "main"))
	{
		nullcLastError = linker->GetLinkError();
		return false;
	}

	nullcLastError = linker->GetLinkError();

	OutputContext outputCtx;

	outputCtx.openStream = openStream;
	outputCtx.writeStream = writeStream;
	outputCtx.closeStream = closeStream;

	outputCtx.outputBuf = outputBuf;
	outputCtx.outputBufSize = NULLC_OUTPUT_BUFFER_SIZE;

	outputCtx.tempBuf = tempOutputBuf;
	outputCtx.tempBufSize = NULLC_TEMP_OUTPUT_BUFFER_SIZE;

	if(enableLogFiles)
	{
		outputCtx.stream = outputCtx.openStream("link_vm.txt");

		if(outputCtx.stream)
		{
			linker->SaveVmListing(outputCtx);

			outputCtx.closeStream(outputCtx.stream);
			outputCtx.stream = NULL;
		}

		outputCtx.stream = outputCtx.openStream("link_reg_vm.txt");

		if(outputCtx.stream)
		{
			linker->SaveRegVmListing(outputCtx, false);

			outputCtx.closeStream(outputCtx.stream);
			outputCtx.stream = NULL;
		}
	}
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
		bool res = executorX86->TranslateToNative(enableLogFiles, outputCtx);
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
	if(currExec == NULLC_REG_VM)
		executor->UpdateInstructionPointer();
#endif

#ifndef NULLC_NO_EXECUTOR
	return true;
#else
	return false;
#endif
}

nullres nullcBuild(const char* code)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcBuild");

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

	TRACE_SCOPE("nullc", "nullcRun");

#ifndef NULLC_NO_EXECUTOR
	return nullcRunFunction(NULL);
#else
	nullcLastError = "No executor available, compile library without NULLC_NO_EXECUTOR";
	return false;
#endif
}

#ifndef NULLC_NO_EXECUTOR
const char*	nullcGetArgumentVector(unsigned functionID, uintptr_t extra, va_list args)
{
	using namespace NULLC;

	// Copy arguments in argument buffer
	ExternFuncInfo	&func = linker->exFunctions[functionID];
	char *argPos = argBuf;
	for(unsigned i = 0; i < func.paramCount; i++)
	{
		ExternLocalInfo &lInfo = linker->exLocals[func.offsetToFirstLocal + i];
		ExternTypeInfo &tInfo = linker->exTypes[lInfo.type];
		switch(tInfo.type)
		{
		case ExternTypeInfo::TYPE_VOID:
			break;
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
			static unsigned autoRefHash = GetStringHash("auto ref");
			static unsigned autoArrayHash = GetStringHash("auto[]");
#ifdef _WIN64
			if(tInfo.size <= 4)
				*(int*)argPos = va_arg(args, int);
			else if(tInfo.size <= 8)
				*(long long*)argPos = va_arg(args, long long);
			else
				memcpy(argPos, va_arg(args, char*), tInfo.size);
			argPos += tInfo.size;
#elif defined(__aarch64__)
			if(tInfo.size <= 4)
			{
				*(int*)argPos = va_arg(args, int);
			}
			else if(tInfo.size <= 8)
			{
				*(long long*)argPos = va_arg(args, long long);
			}
			else if(tInfo.size <= 12)
			{
				*(long long*)argPos = va_arg(args, long long);
				*(long long*)(argPos + 8) = va_arg(args, long long);
			}
			else if(tInfo.size <= 16)
			{
				*(long long*)argPos = va_arg(args, long long);
				*(long long*)(argPos + 8) = va_arg(args, long long);
			}
			else
			{
				memcpy(argPos, va_arg(args, char*), tInfo.size);
			}
			argPos += tInfo.size;
#elif defined(__x86_64__)
			if((tInfo.subCat == ExternTypeInfo::CAT_ARRAY && tInfo.arrSize == ~0u) || tInfo.subCat == ExternTypeInfo::CAT_FUNCTION)
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
				ExternMemberInfo *memberList = &linker->exTypeExtra[tInfo.memberOffset];
				for(unsigned k = 0; k < tInfo.memberCount; k++)
				{
					const ExternTypeInfo &subType = linker->exTypes[memberList[k].type];
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
			for(unsigned u = 0; u < tInfo.size >> 2; u++, argPos += 4)
				*(int*)argPos = va_arg(args, int);
#endif
			break;
		}
	}

	memcpy(argPos, &extra, sizeof(extra));
	argPos += sizeof(uintptr_t);

	return argBuf;
}
#endif

nullres	nullcRunFunction(const char* funcName, ...)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	unsigned functionID = ~0u;

#ifndef NULLC_NO_EXECUTOR
	const char* argBuf = NULL;

	// If function is called, find it's index
	if(funcName)
	{
		functionID = nullcFindFunctionIndex(funcName);
		if(functionID == ~0u)
			return false;

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

	return nullcRunFunctionInternal(functionID, argBuf);
}

nullres nullcRunFunctionInternal(unsigned functionID, const char* argBuf)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	nullres good = true;

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
	}
	else if(currExec == NULLC_X86)
	{
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
	}
	else if(currExec == NULLC_LLVM)
	{
		executorLLVM->Run(functionID, argBuf);
		const char* error = executorLLVM->GetExecError();
		if(error[0] != '\0')
		{
			good = false;
			nullcLastError = error;
		}
#endif
	}
	else if(currExec == NULLC_REG_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		executorRegVm->Run(functionID, argBuf);
		const char* error = executorRegVm->GetExecError();
		if(error[0] != '\0')
		{
			good = false;
			nullcLastError = error;
		}
#endif
	}
	else
	{
		(void)functionID;
		(void)argBuf;

		good = false;
		nullcLastError = "Unknown executor code";
	}

#if !defined(NULLC_NO_EXECUTOR) && defined(NULLC_REG_VM_PROFILE_INSTRUCTIONS)
	if(currExec == NULLC_REG_VM && enableLogFiles)
	{
		OutputContext outputCtx;

		outputCtx.openStream = openStream;
		outputCtx.writeStream = writeStream;
		outputCtx.closeStream = closeStream;

		outputCtx.outputBuf = outputBuf;
		outputCtx.outputBufSize = NULLC_OUTPUT_BUFFER_SIZE;

		outputCtx.tempBuf = tempOutputBuf;
		outputCtx.tempBufSize = NULLC_TEMP_OUTPUT_BUFFER_SIZE;

		outputCtx.stream = outputCtx.openStream("link_reg_vm_exec.txt");

		if(outputCtx.stream)
		{
			linker->SaveRegVmListing(outputCtx, true);

			outputCtx.closeStream(outputCtx.stream);
			outputCtx.stream = NULL;
		}
	}
#endif

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

	va_end(args);

	if(currExec == NULLC_VM)
	{
		executor->Stop(buf);
	}
	else if(currExec == NULLC_X86)
	{
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Stop(buf);
#endif
	}
	else if(currExec == NULLC_REG_VM)
	{
		executorRegVm->Stop(buf);
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
	va_end(args);

	if(!argBuf)
		return false;
	
	if(currExec == NULLC_VM)
	{
		executor->Run(ptr.id, argBuf);
		error = executor->GetExecError();
	}
	else if(currExec == NULLC_X86)
	{
#ifdef NULLC_BUILD_X86_JIT
		executorX86->Run(ptr.id, argBuf);
		error = executorX86->GetExecError();
#endif
	}
	else if(currExec == NULLC_LLVM)
	{
#ifdef NULLC_LLVM_SUPPORT
		executorLLVM->Run(ptr.id, argBuf);
		error = executorLLVM->GetExecError();
#endif
	}
	else if(currExec == NULLC_REG_VM)
	{
		executorRegVm->Run(ptr.id, argBuf);
		error = executorRegVm->GetExecError();
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
	unsigned hash = GetStringHash(name);
	for(unsigned i = 0; i < linker->exVariables.size(); i++)
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
	NULLC_CHECK_INITIALIZED(0);

	char* mem = (char*)nullcGetVariableData(NULL);
	if(!linker || !name || !mem)
		return NULL;
	unsigned hash = GetStringHash(name);
	for(unsigned i = 0; i < linker->exVariables.size(); i++)
	{
		if(linker->exVariables[i].nameHash == hash)
			return mem + linker->exVariables[i].offset;
	}
	return NULL;
}

unsigned nullcGetGlobalType(const char* name)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	char* mem = (char*)nullcGetVariableData(NULL);
	if(!linker || !name || !mem)
		return 0;
	unsigned hash = GetStringHash(name);
	for(unsigned i = 0; i < linker->exVariables.size(); i++)
	{
		if(linker->exVariables[i].nameHash == hash)
			return linker->exVariables[i].type;
	}
	return 0;
}

unsigned nullcFindFunctionIndex(const char* name)
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
	unsigned hash = GetStringHash(name);
	unsigned index = ~0u;
	for(unsigned i = 0; i < linker->exFunctions.size(); i++)
	{
		if(linker->exFunctions[i].isVisible && linker->exFunctions[i].nameHash == hash)
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
	unsigned index = nullcFindFunctionIndex(name);
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

	unsigned index = nullcFindFunctionIndex(name);
	if(index == ~0u)
		return false;

	if(linker->exFunctions[func.id].funcCat != ExternFuncInfo::NORMAL)
	{
		nullcLastError = "ERROR: source function uses context, which is unavailable";
		return false;
	}

	return nullcRedirectFunction(index, func.id);
}

nullres nullcRedirectFunction(unsigned sourceId, unsigned targetId)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
		executorX86->UpdateFunctionPointer(sourceId, targetId);
#endif

	ExternFuncInfo &destFunc = linker->exFunctions[sourceId];
	ExternFuncInfo &srcFunc = linker->exFunctions[targetId];

	destFunc.vmAddress = srcFunc.vmAddress;
	destFunc.vmCodeSize = srcFunc.vmCodeSize;

	destFunc.regVmAddress = srcFunc.regVmAddress;
	destFunc.regVmCodeSize = srcFunc.regVmCodeSize;
	destFunc.regVmRegisters = srcFunc.regVmRegisters;

	destFunc.funcPtrRaw = srcFunc.funcPtrRaw;
	destFunc.funcPtrWrapTarget = srcFunc.funcPtrWrapTarget;
	destFunc.funcPtrWrap = srcFunc.funcPtrWrap;

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
#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_REG_VM)
		return executorRegVm->GetResult();
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
#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_REG_VM)
		return executorRegVm->GetResultInt();
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
#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_REG_VM)
		return executorRegVm->GetResultDouble();
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
#ifndef NULLC_NO_EXECUTOR
	if(currExec == NULLC_REG_VM)
		return executorRegVm->GetResultLong();
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

	TRACE_SCOPE("nullc", "nullcFinalize");

	FinalizeMemory();

	return 1;
}

void* nullcAllocate(unsigned size)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	return NULLC::AllocObject(size);
}

void* nullcAllocateTyped(unsigned typeID)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(0);

	return NULLC::AllocObject(nullcGetTypeSize(typeID), typeID);
}

NULLCArray nullcAllocateArrayTyped(unsigned typeID, unsigned count)
{
	using namespace NULLC;
	NULLCArray arr = { 0, 0 };
	NULLC_CHECK_INITIALIZED(arr);
	
	return NULLC::AllocArray(nullcGetTypeSize(typeID), count, typeID);
}

int nullcInitTypeinfoModule()
{
	TRACE_SCOPE("nullc", "nullcInitTypeinfoModule");

	return nullcInitTypeinfoModule(NULLC::linker);
}

int nullcInitDynamicModule()
{
	TRACE_SCOPE("nullc", "nullcInitDynamicModule");

	return nullcInitDynamicModule(NULLC::linker);
}
#endif

void nullcTerminate()
{
	using namespace NULLC;
	nullcLastError = "";
	if(!initialized)
		return;

	NULLC::destruct(compilerCtx);
	compilerCtx = NULL;

	allocator.Reset();

	NULLC::dealloc(argBuf);
	argBuf = NULL;

	NULLC::dealloc(errorBuf);
	errorBuf = NULL;

	NULLC::dealloc(outputBuf);
	outputBuf = NULL;

	NULLC::dealloc(tempOutputBuf);
	tempOutputBuf = NULL;

	BinaryCache::Terminate();

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
	NULLC::destruct(executorRegVm);
	executorRegVm = NULL;
#endif

#ifndef NULLC_NO_EXECUTOR
	NULLC::ResetMemory();
#endif

	initialized = false;
}

nullres nullcTestEvaluateExpressionTree(char *resultBuf, unsigned resultBufSize)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcTestEvaluateExpressionTree");

	if(!compilerCtx)
	{
		nullcLastError = "ERROR: there is no active compiler context";
		return 0;
	}

	if(!TestEvaluation(compilerCtx->exprCtx, compilerCtx->exprModule, resultBuf, resultBufSize, compilerCtx->errorBuf, compilerCtx->errorBufSize))
	{
		nullcLastError = compilerCtx->errorBuf;
		return false;
	}

	return true;
}

nullres nullcTestEvaluateInstructionTree(char *resultBuf, unsigned resultBufSize)
{
	using namespace NULLC;
	NULLC_CHECK_INITIALIZED(false);

	TRACE_SCOPE("nullc", "nullcTestEvaluateInstructionTree");

	if(!compilerCtx)
	{
		nullcLastError = "ERROR: there is no active compiler context";
		return 0;
	}

	if(!TestEvaluation(compilerCtx->exprCtx, compilerCtx->vmModule, resultBuf, resultBufSize, compilerCtx->errorBuf, compilerCtx->errorBufSize))
	{
		nullcLastError = compilerCtx->errorBuf;
		return false;
	}

	return true;
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
	}
	else if(currExec == NULLC_X86)
	{
#ifdef NULLC_BUILD_X86_JIT
		return executorX86->GetVariableData(count);
#endif
#if defined(NULLC_LLVM_SUPPORT) && !defined(NULLC_NO_EXECUTOR)
	}
	else if(currExec == NULLC_LLVM)
	{
		return executorLLVM->GetVariableData(count);
#endif
	}
	else if(currExec == NULLC_REG_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		return executorRegVm->GetVariableData(count);
#endif
	}

	(void)count;
	return NULL;
}

unsigned int nullcGetCurrentExecutor(void **exec)
{
	using namespace NULLC;

#if !defined(NULLC_NO_EXECUTOR)
	if(exec)
		*exec = (currExec == NULLC_VM ? (void*)executor : (currExec == NULLC_X86 ? (void*)executorX86 : (currExec == NULLC_LLVM ? (void*)executorLLVM : (void*)executorRegVm)));
#else
	*exec = NULL;
#endif

	return currExec;
}

const void* nullcGetModule(const char* path)
{
	return BinaryCache::FindBytecode(path, false);
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

ExternSourceInfo* nullcDebugSourceInfo(unsigned int *count)
{
	using namespace NULLC;

	if(nullcGetCurrentExecutor(NULL) == NULLC_REG_VM || nullcGetCurrentExecutor(NULL) == NULLC_X86)
	{
		if(count && linker)
			*count = linker->exRegVmSourceInfo.size();
		return linker ? (ExternSourceInfo*)linker->exRegVmSourceInfo.data : NULL;
	}

	if(count && linker)
		*count = linker->exVmSourceInfo.size();
	return linker ? (ExternSourceInfo*)linker->exVmSourceInfo.data : NULL;
}

VMCmd* nullcDebugCode(unsigned int *count)
{
	using namespace NULLC;

	if(count && linker)
		*count = linker->exVmCode.size();
	return linker ? (VMCmd*)linker->exVmCode.data : NULL;
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
	}
	else if(currExec == NULLC_X86)
	{
#ifdef NULLC_BUILD_X86_JIT
		executorX86->BeginCallStack();
#endif
	}
	else if(currExec == NULLC_REG_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		executorRegVm->BeginCallStack();
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
	}
	else if(currExec == NULLC_X86)
	{
#ifdef NULLC_BUILD_X86_JIT
		address = executorX86->GetNextAddress();
#endif
	}
	else if(currExec == NULLC_REG_VM)
	{
#ifndef NULLC_NO_EXECUTOR
		address = executorRegVm->GetNextAddress();
#endif
	}

	return address;
}

#ifndef NULLC_NO_EXECUTOR
nullres nullcDebugSetBreakFunction(void *context, unsigned (*callback)(void*, unsigned))
{
	using namespace NULLC;

	if(!executor)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executor->SetBreakFunction(context, callback);

#ifdef NULLC_BUILD_X86_JIT
	if(!executorX86)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executorX86->SetBreakFunction(context, callback);
#endif

	if(!executorRegVm)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executorRegVm->SetBreakFunction(context, callback);

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

	if(!executorRegVm)
	{
		nullcLastError = "ERROR: NULLC is not initialized";
		return false;
	}
	executorRegVm->ClearBreakpoints();

	return true;
}

nullres nullcDebugAddBreakpointImpl(unsigned int instruction, bool oneHit)
{
	using namespace NULLC;

	if(currExec == NULLC_VM)
	{
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
	}

#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
	{
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
	}
#endif

	if(currExec == NULLC_REG_VM)
	{
		if(!executorRegVm)
		{
			nullcLastError = "ERROR: NULLC is not initialized";
			return false;
		}

		if(!executorRegVm->AddBreakpoint(instruction, oneHit))
		{
			nullcLastError = executorRegVm->GetExecError();
			return false;
		}
	}

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

	if(currExec == NULLC_VM)
	{
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
	}

#ifdef NULLC_BUILD_X86_JIT
	if(currExec == NULLC_X86)
	{
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
	}
#endif

	if(currExec == NULLC_REG_VM)
	{
		if(!executorRegVm)
		{
			nullcLastError = "ERROR: NULLC is not initialized";
			return false;
		}
		if(!executorRegVm->RemoveBreakpoint(instruction))
		{
			nullcLastError = executorRegVm->GetExecError();
			return false;
		}
	}

	return true;
}

ExternFuncInfo* nullcDebugConvertAddressToFunction(int instruction, ExternFuncInfo* codeFunctions, unsigned functionCount)
{
	using namespace NULLC;

	if(currExec == NULLC_VM)
	{
		for(unsigned i = 0; i < functionCount; i++)
		{
			if(instruction >= codeFunctions[i].vmAddress && instruction < (codeFunctions[i].vmAddress + codeFunctions[i].vmCodeSize))
				return &codeFunctions[i];
		}
	}
	else if(currExec == NULLC_REG_VM || currExec == NULLC_X86)
	{
		for(unsigned i = 0; i < functionCount; i++)
		{
			if(instruction >= codeFunctions[i].regVmAddress && instruction < (codeFunctions[i].regVmAddress + codeFunctions[i].regVmCodeSize))
				return &codeFunctions[i];
		}
	}

	return NULL;
}

#endif

CompilerContext* nullcGetCompilerContext()
{
	return NULLC::compilerCtx;
}

void nullcVisitParseTreeNodes(SynBase *syntax, void *context, void(*accept)(void *context, SynBase *child))
{
	TRACE_SCOPE("nullc", "nullcVisitParseTreeNodes");

	VisitParseTreeNodes(syntax, context, accept);
}

void nullcVisitExpressionTreeNodes(ExprBase *expression, void *context, void(*accept)(void *context, ExprBase *child))
{
	TRACE_SCOPE("nullc", "nullcVisitExpressionTreeNodes");

	VisitExpressionTreeNodes(expression, context, accept);
}
