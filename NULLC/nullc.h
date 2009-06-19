#ifndef NULLC_INCLUDED
#define NULLC_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _MSC_VER
	#define NCDECL _cdecl
#else
	#define NCDECL
#endif

typedef unsigned char nullres;

void	nullcInit();

nullres	nullcCompile(const char* code);
const char*	nullcGetCompilationError();
const char*	nullcGetCompilationLog();
const char*	nullcGetListing();

nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype);

void*	nullcGetVariableDataX86();
nullres	nullcTranslateX86(int optimised);
nullres	nullcExecuteX86(unsigned int* runTime);

void*	nullcGetVariableDataVM();
nullres	nullcExecuteVM(unsigned int* runTime, nullres (*func)(unsigned int));

const char*	nullcGetExecutionLog();
const char*	nullcGetResult();

void**	nullcGetVariableInfo(unsigned int* count);

void	nullcDeinit();

#ifdef __cplusplus
}
#endif

#endif