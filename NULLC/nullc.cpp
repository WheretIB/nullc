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
std::vector<shared_ptr<NodeZeroOP> >	CodeInfo::nodeList;
ostringstream				CodeInfo::compileLog;
unsigned int				CodeInfo::globalSize = 0;

Compiler*	compiler;
Linker*		linker;
Executor*	executor;
#ifdef NULLC_BUILD_X86_JIT
	ExecutorX86*	executorX86;
#endif

std::ostringstream strStream;
std::string	compileError;
std::string	compileLogString;
std::string	compileListing;

const char* executeResult;
std::string executeLog;

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
		strStream.str("");
		strStream << err;
		compileError = strStream.str();
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
	compileLogString = CodeInfo::compileLog.str();
	return compileLogString.c_str();
}

const char*	nullcGetListing()
{
	compiler->GenListing();
	compileListing = compiler->GetListing();
	return compileListing.c_str();
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
		try
		{
			executorX86->SetOptimization(execOptimize);
			executorX86->GenListing();
		}catch(const std::string& str){
			executeLog += "    " + str;
			return false;
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
	delete executor;
#ifdef NULLC_BUILD_X86_JIT
	delete executorX86;
#endif
	delete CodeInfo::cmdInfoList;
}