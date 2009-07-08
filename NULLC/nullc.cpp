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

std::vector<FunctionInfo*>	CodeInfo::funcInfo;
std::vector<VariableInfo*>	CodeInfo::varInfo;
std::vector<TypeInfo*>		CodeInfo::typeInfo;
CommandList*				CodeInfo::cmdInfoList;
FastVector<VMCmd>			CodeInfo::cmdList;
std::vector<NodeZeroOP*>	CodeInfo::nodeList;
unsigned int				CodeInfo::globalSize = 0;

Compiler*	compiler;
Linker*		linker;
Executor*	executor;
#ifdef NULLC_BUILD_X86_JIT
	ExecutorX86*	executorX86;
#endif

std::string	compileError;

const char* executeResult;
std::string executeLog;

char* compileLog;

unsigned int currExec = 0;
bool	execOptimize = false;

void	nullcInit()
{
	CodeInfo::cmdInfoList = new CommandList();

	compiler = new Compiler();
	linker = new Linker();
	executor = new Executor(linker);
#ifdef NULLC_BUILD_X86_JIT
	executorX86 = new ExecutorX86(linker);
	executorX86->Initialize();
#endif
#ifdef NULLC_LOG_FILES
	compileLog = NULL;
#endif
}

void	nullcSetExecutor(unsigned int id)
{
	currExec = id;
	CodeInfo::activeExecutor = currExec;
}

void	nullcSetExecutorOptions(int optimize)
{
	execOptimize = !!optimize;
}

nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	return compiler->AddExternalFunction(ptr, prototype);
}

nullres	nullcCompile(const char* code)
{
	compileError = "";
	nullres good = false;
	try
	{
		good = compiler->Compile(code);
	}catch(const std::string& str){
		good = false;
		compileError = str;
	}catch(const CompilerError& err){
		good = false;
		compileError = err.GetErrorString();
	}
	return good;
}

const char*	nullcGetCompilationError()
{
	return compileError.c_str();
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
	delete[] compileLog;
	compileLog = new char[size+1];
	fread(compileLog, 1, size, cLog);
	compileLog[size] = 0;
	return compileLog;
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

void nullcFixupBytecode(char *bytecode)
{
	BytecodeFixup((ByteCode*)bytecode);
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
		executorX86->SetOptimization(execOptimize);
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
	return executeLog.c_str();
}

nullres	emptyCallback(unsigned int)
{
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
		executor->SetCallback((bool (*)(unsigned int))(emptyCallback));
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
	return executeLog.c_str();
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
	delete linker;
	delete executor;
#ifdef NULLC_BUILD_X86_JIT
	delete executorX86;
#endif
#ifdef NULLC_LOG_FILES
	delete[] compileLog;
#endif
	delete CodeInfo::cmdInfoList;
}