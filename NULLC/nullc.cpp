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
std::string	compileLogString;
std::string	compileListing;

std::string executeResult, executeLog;

void	nullcInit()
{
	CodeInfo::cmdList = new CommandList();

	compiler = new Compiler();
	executor = new Executor();
	executorX86 = new ExecutorX86();
}

nullres	nullcCompile(const char* code)
{
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


nullres	nullcAddExternalFunction(void (_cdecl *ptr)(), const char* prototype)
{
	return compiler->AddExternalFunction(ptr, prototype);
}

void*	nullcGetVariableDataX86()
{
	return executorX86->GetVariableData();
}

nullres	nullcTranslateX86(int optimised)
{
	nullres good = true;
	try
	{
		executorX86->SetOptimization(optimised);
		executorX86->GenListing();
	}catch(const std::string& str){
		good = false;
		executeLog = str;
	}
	return good;
}

nullres	nullcExecuteX86(unsigned int* runTime)
{
	nullres good = true;
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

nullres	emptyCallback(unsigned int)
{
	return true;
}

nullres	nullcExecuteVM(unsigned int* runTime, nullres (*func)(unsigned int), const char* funcName)
{
	nullres good = true;
	try
	{
		executor->SetCallback((bool (*)(unsigned int))(func ? func : emptyCallback));
		*runTime = executor->Run(funcName);
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