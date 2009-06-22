#include "stdafx.h"
#include "nullc.h"

#include "CodeInfo.h"

#include "Compiler.h"
#include "Executor.h"
#ifdef NULLC_BUILD_X86_JIT
	#include "Executor_X86.h"
#endif

unsigned int CodeInfo::activeExecutor = 0;

std::vector<FunctionInfo*>	CodeInfo::funcInfo;
std::vector<VariableInfo*>	CodeInfo::varInfo;
std::vector<TypeInfo*>		CodeInfo::typeInfo;
CommandList*				CodeInfo::cmdList;
std::vector<shared_ptr<NodeZeroOP> >	CodeInfo::nodeList;
ostringstream				CodeInfo::compileLog;

Compiler*	compiler;
Executor*	executor;
#ifdef NULLC_BUILD_X86_JIT
	ExecutorX86*	executorX86;
#endif

std::ostringstream strStream;
std::string	compileError;
std::string	compileLogString;
std::string	compileListing;

std::string executeResult, executeLog;

unsigned int currExec = 0;
bool	optimize = false;

void	nullcInit()
{
	CodeInfo::cmdList = new CommandList();

	compiler = new Compiler();
	executor = new Executor();
#ifdef NULLC_BUILD_X86_JIT
	executorX86 = new ExecutorX86();
#endif
}

void	nullcSetExecutor(unsigned int id)
{
	currExec = id;
	CodeInfo::activeExecutor = currExec;
}

void	nullcSetExecutorOptions(int optimize)
{
	optimize = true;
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
	if(good && currExec == NULLC_X86)
	{
#ifdef NULLC_BUILD_X86_JIT
		try
		{
			executorX86->SetOptimization(optimize);
			executorX86->GenListing();
		}catch(const std::string& str){
			good = false;
			compileError += "    " + str;
		}
#else
		good = false;
		compileError = "X86 JIT isn't available";
#endif
	}
	return good;
}

const char*	nullcGetCompilationError()
{
	return compileError.c_str();
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
		try
		{
			executorX86->Run(funcName);
			executeResult = executorX86->GetResult();
		}catch(const std::string& str){
			good = false;
			executeLog = str;
		}
#else
		good = false;
		executeLog = "X86 JIT isn't available";
#endif
	}else{
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
	return executeResult.c_str();
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
	delete CodeInfo::cmdList;
}