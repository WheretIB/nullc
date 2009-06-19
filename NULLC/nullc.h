#ifndef NULLC_INCLUDED
#define NULLC_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

void	nullcInit();

bool	nullcCompile(const char* code);
const char*	nullcGetCompilationError();
const char*	nullcGetCompilationLog();
const char*	nullcGetListing();

bool	nullcAddExternalFunction(void (_cdecl *ptr)(), const char* prototype);

void*	nullcGetVariableDataX86();
bool	nullcTranslateX86(int optimised);
bool	nullcExecuteX86(unsigned int* runTime);

void*	nullcGetVariableDataVM();
bool	nullcExecuteVM(unsigned int* runTime, bool (*func)(unsigned int));

const char*	nullcGetExecutionLog();
const char*	nullcGetResult();

void**	nullcGetVariableInfo(unsigned int* count);

void	nullcDeinit();

#ifdef __cplusplus
}
#endif

#endif