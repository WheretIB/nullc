#ifndef NULLC_DEBUG_INCLUDED
#define NULLC_DEBUG_INCLUDED

#include "nullcdef.h"
#include "Bytecode.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/*							Debug functions								*/

void*				nullcGetVariableData();

unsigned int		nullcGetCurrentExecutor(void **exec);
const void*			nullcGetModule(const char* path);

/*	Used to retrieve code information of linked code	*/

ExternTypeInfo*		nullcDebugTypeInfo(unsigned int *count);
unsigned int*		nullcDebugTypeExtraInfo(unsigned int *count);
ExternVarInfo*		nullcDebugVariableInfo(unsigned int *count);
ExternFuncInfo*		nullcDebugFunctionInfo(unsigned int *count);
ExternLocalInfo*	nullcDebugLocalInfo(unsigned int *count);
char*				nullcDebugSymbols();

void				nullcDebugBeginCallStack();
unsigned int		nullcDebugGetStackFrame();

/*
void				nullcDebugClearBreakpoints();
void				nullcDebugAddBreakpoint(unsigned int line);
void				nullcDebugRemoveBreakpoint(unsigned int line);
*/

#ifdef __cplusplus
}
#endif

#endif
