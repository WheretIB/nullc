#include "nullc.h"

#include "CodeInfo.h"

#include "Compiler.h"
#include "Executor.h"
#include "Executor_X86.h"

unsigned int CodeInfo::activeExecutor = 0;

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

unsigned int currExec = 0;
bool	optimize = false;

void	nullcInit()
{
	CodeInfo::cmdList = new CommandList();

	compiler = new Compiler();
	executor = new Executor();
	executorX86 = new ExecutorX86();
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

nullres	nullcAddExternalFunction(void (_cdecl *ptr)(), const char* prototype)
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
		try
		{
			executorX86->SetOptimization(optimize);
			executorX86->GenListing();
		}catch(const std::string& str){
			good = false;
			compileError += "    " + str;
		}
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

nullres	nullcRun(unsigned int* runTime)
{
	return nullcRunFunction(runTime, NULL);
}

nullres	nullcRunFunction(unsigned int* runTime, const char* funcName)
{
	nullres good = true;
	if(currExec == NULLC_VM)
	{
		try
		{
			executor->SetCallback((bool (*)(unsigned int))(emptyCallback));
			*runTime = executor->Run(funcName);
			executeResult = executor->GetResult();
		}catch(const std::string& str){
			good = false;
			executeLog = str;
		}
	}else if(currExec == NULLC_X86){
		try
		{
			*runTime = executorX86->Run(funcName);
			executeResult = executorX86->GetResult();
		}catch(const std::string& str){
			good = false;
			executeLog = str;
		}
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
		return executorX86->GetVariableData();
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
	delete executorX86;
	delete CodeInfo::cmdList;
}