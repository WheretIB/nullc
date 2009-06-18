#include "nullc.h"

#include "CodeInfo.h"

#include "Compiler.h"
#include "Executor.h"
#include "Executor_X86.h"

std::vector<FunctionInfo*>	CodeInfo::funcInfo;
std::vector<VariableInfo*>	CodeInfo::varInfo;
std::vector<TypeInfo*>		CodeInfo::typeInfo;
CommandList*				CodeInfo::cmdList;
std::vector<shared_ptr<NodeZeroOP> >	CodeInfo::nodeList;
ostringstream				CodeInfo::compileLog;

Compiler*	compiler;
Executor*	executor;
ExecutorX86*	executorX86;

std::ostringstream strStream;
std::string	compileError;
std::string	compileLog;
std::string	compileListing;

std::string executeResult, executeLog;

void	nullcInit()
{
	CodeInfo::cmdList = new CommandList();

	compiler = new Compiler();
	executor = new Executor();
	executorX86 = new ExecutorX86();
}

bool	nullcCompile(const char* code)
{
	bool good = false;
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

const char*	nullcGetCompilationLog()
{
	compileLog = CodeInfo::compileLog.str();
	return compileLog.c_str();
}

const char*	nullcGetListing()
{
	compiler->GenListing();
	compileListing = compiler->GetListing();
	return compileListing.c_str();
}


bool	nullcAddExternalFunction(void (_cdecl *ptr)(), const char* prototype)
{
	return compiler->AddExternalFunction(ptr, prototype);
}

void*	nullcGetVariableDataX86()
{
	return executorX86->GetVariableData();
}

bool	nullcTranslateX86(int optimised)
{
	bool good = true;
	try
	{
		executorX86->GenListing();
	}catch(const std::string& str){
		good = false;
		executeLog = str;
	}
	return good;
}

bool	nullcExecuteX86(unsigned int* runTime)
{
	bool good = true;
	try
	{
		*runTime = executorX86->Run();
		executeResult = executorX86->GetResult();
	}catch(const std::string& str){
		good = false;
		executeLog = str;
	}
	return good;
}

void*	nullcGetVariableDataVM()
{
	return executor->GetVariableData();
}

bool	nullcExecuteVM(unsigned int* runTime, bool (*func)(unsigned int))
{
	bool good = true;
	try
	{
		executor->SetCallback(func);
		*runTime = executor->Run();
		executeResult = executor->GetResult();
	}catch(const std::string& str){
		good = false;
		executeLog = str;
	}
	return good;
}

const char*	nullcGetExecutionLog()
{
	return executeLog.c_str();
}

const char*	nullcGetResult()
{
	return executeResult.c_str();
}

void**	nullcGetVariableInfo(unsigned int* count)
{
	*count = CodeInfo::varInfo.size();
	return (void**)(&CodeInfo::varInfo[0]);
}

void	nullcDeinit()
{
	delete compiler;
	delete executor;
	delete executorX86;
	delete CodeInfo::cmdList;
}