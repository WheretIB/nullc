#ifndef NULLC_INCLUDED
#define NULLC_INCLUDED

#include "nullcdef.h"

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

// Initialize NULLC
void	nullcInit();

#define NULLC_VM	0
#define NULLC_X86	1
void	nullcSetExecutor(unsigned int id);
void	nullcSetExecutorOptions(int optimize);

nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype);

nullres	nullcCompile(const char* code);
const char*	nullcGetCompilationError();
const char*	nullcGetCompilationLog();
const char*	nullcGetListing();

nullres	nullcRun(unsigned int* runTime);
nullres	nullcRunFunction(unsigned int* runTime, const char* funcName);
const char*	nullcGetRuntimeError();

const char*	nullcGetResult();
void*	nullcGetVariableData();

void	nullcDeinit();

void**	nullcGetVariableInfo(unsigned int* count);

#ifdef __cplusplus
}
#endif

#endif